#include "NetSocket.h"
#include "../IO/Log.h"

bool MakeSocketNonBlocking(socket_t fd)
{
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket(fd, FIONBIO, &mode) == 0;
#else
    int32 flags = fcntl(fd, F_GETFL, 0);
    if(flags == -1) {
        return false;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
#endif
}

void CloseSocket(socket_t fd)
{
#ifdef _WIN32
    closesocket(fd);
#else
    close(fd);
#endif
}

string GetIPFromSocket(socket_t socket)
{
    sockaddr_in addr;
#ifdef _WIN32
    int32 len = sizeof(addr);
#else
    socklen_t len = sizeof(addr);
#endif

    if(getpeername(socket, (sockaddr*)&addr, &len) != 0) {
        return "";
    }

#ifdef _WIN32
    return inet_ntoa(addr.sin_addr);
#else
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip, sizeof(ip));

    return ip;
#endif
}

int GetIPStringFromHost(uint32 host, char* buffer, uint32 bufferSize)
{
    if(!buffer || bufferSize < 16) 
        return -1;

    struct in_addr addr;
    addr.s_addr = host;

    if(inet_ntop(AF_INET, &addr, buffer, bufferSize) != nullptr)
    {
        return 0;
    }

    return -1;
}

NetSocket::NetSocket()
: m_socket(SOCKET_INVALID), m_lastConnID(0)
{
#ifdef SOCKET_USE_TLS
    m_pSslCtx = nullptr;
#endif
}

NetSocket::~NetSocket()
{
    Kill();
}

bool NetSocket::Init(const string& host, uint16 port, int32 backLog)
{
#ifdef _WIN32
    WSADATA wsaData;
    if(WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        LOGGER_LOG_ERROR("WSAStartup failed.");
        return false;
    }
#endif

    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(m_socket < 0) {
        return false;
    }

#ifdef _DEBUG
    int opt = 1;
    setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

    linger lin;
    lin.l_onoff = 0;
    lin.l_linger = 0;
    setsockopt(m_socket, SOL_SOCKET, SO_LINGER, (const char*)&lin, sizeof(lin));
#endif

    sockaddr_in sockAddr{};
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(port);

    if(host.empty()) {
        sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else {
        uint32 addr = inet_addr(host.c_str());
    
        if(addr == INADDR_NONE) {
            hostent* hNet = gethostbyname(host.c_str());
            if(!hNet) {
                return false;
            }
    
            sockAddr.sin_addr = *(in_addr*)hNet->h_addr;
        }
        else {
            sockAddr.sin_addr.s_addr = addr;
        }
    }

    if(bind(m_socket, (sockaddr*)&sockAddr, sizeof(sockAddr)) < 0) {
        return false;
    }

    if(backLog > 0) {
        if(listen(m_socket, backLog) < 0) {
            return false;
        }
    }

    if(!MakeSocketNonBlocking(m_socket)) {
        return false;
    }

    return true;
}

int16 NetSocket::Connect(const string& host, uint16 port, bool nonBlocking)
{
    sockaddr_in sockAddr{};
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(port);
    
    uint32 addr = inet_addr(host.c_str());
    if(addr != INADDR_NONE) {
        sockAddr.sin_addr.s_addr = addr;
    }
    else {
        hostent* hNet = gethostbyname(host.c_str());
        if(!hNet || !hNet->h_addr_list[0]) {
            return -2;
        }
    
        sockAddr.sin_addr = *(in_addr*)hNet->h_addr;
    }

    socket_t socketCli = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(nonBlocking) {
        if(!MakeSocketNonBlocking(socketCli)) {
            return -2;
        }
    }

    int32 result = connect(socketCli, (sockaddr*)&sockAddr, sizeof(sockAddr));
    if(!nonBlocking && result < 0) {
        return -1;
    }

#ifdef SOCKET_USE_TLS
    SSL* pSsl = SSL_new(m_pSslCtx); 
    if(!pSsl) {
        CloseSocket(socketCli);
        SSL_free(pSsl);
        return -1;
    }

    SSL_set_tlsext_host_name(pSsl, host.c_str()); // here
    SSL_set_fd(pSsl, socketCli);

    if(nonBlocking) 
    {
        SSL_set_connect_state(pSsl);
        int32 sslRes = SSL_connect(pSsl);
        int32 sslErr = SSL_get_error(pSsl, sslRes);
        
        if(sslRes <= 0 && sslErr != SSL_ERROR_WANT_READ && sslErr != SSL_ERROR_WANT_WRITE) 
        {
            SSL_free(pSsl);
            CloseSocket(socketCli);
            return -1;
        }
    } 
    else 
    {
        if(SSL_connect(pSsl) <= 0) 
        {
            SSL_free(pSsl);
            CloseSocket(socketCli);
            return -1;
        }
    }
#endif

    NetClient* pClient = new NetClient();
    pClient->connectionID = m_lastConnID++;
    pClient->status = result == 0 ? SOCKET_CLIENT_CONNECTED : SOCKET_CLIENT_CONNECTING;
    pClient->pNetSocket = this;
    pClient->socket = socketCli;
    pClient->ip = GetIPFromSocket(socketCli);

#ifdef SOCKET_USE_TLS
    pClient->socket = SSL_get_fd(pSsl);
    pClient->pSsl = pSsl;
#endif

    m_clients.insert_or_assign(pClient->connectionID, pClient);

    if(socketCli >= (socket_t)m_fdToClient.size()) 
    {
        m_fdToClient.resize(socketCli + 128, nullptr);
    }
    m_fdToClient[socketCli] = pClient;
    
    m_isPollDirty = true; 

    if(!nonBlocking && result >= 0) 
    {
        m_events.Dispatch(SOCKET_EVENT_TYPE_CONNECT, pClient);
    }
    return pClient->connectionID;
}

void NetSocket::Kill()
{
    CloseAllClients();
    CloseSocket(m_socket);

#ifdef SOCKET_USE_TLS
    SSL_CTX_free(m_pSslCtx);
    m_pSslCtx = nullptr;
#endif
}

void NetSocket::CreateSSLCtx()
{
#ifdef SOCKET_USE_TLS
    if(m_pSslCtx) {
        return;
    }
  
    SSL_library_init();
    OpenSSL_add_all_algorithms();

    const SSL_METHOD* sslMethod = TLS_client_method();
    m_pSslCtx = SSL_CTX_new(sslMethod);
#endif
}

void NetSocket::Update(bool asClient)
{
    if(m_isPollDirty) 
    {
        m_pollFds.clear();
        
        if(!asClient && m_socket != SOCKET_INVALID) 
        {
            pollfd listenFd{};
            listenFd.fd = m_socket;
            listenFd.events = POLLIN;
            m_pollFds.push_back(listenFd);
        }

        for(auto& [_, pClient] : m_clients) 
        {
            if(!pClient || pClient->status == SOCKET_CLIENT_CLOSE) 
                continue;

            pollfd cFd{};
            cFd.fd = pClient->socket;
            cFd.events = 0;
            
        #ifdef SOCKET_USE_TLS
            if(pClient->pSsl && (pClient->sslWantsRead || pClient->sslWantsWrite)) 
            {
                if(pClient->sslWantsRead)
                {
                    cFd.events |= POLLIN;
                }

                if(pClient->sslWantsWrite) 
                {
                    cFd.events |= POLLOUT;
                }
            }
            else
        #endif
            {
                cFd.events |= POLLIN;
                
                if(pClient->sendQueue.GetDataSize() > 0 || pClient->status == SOCKET_CLIENT_CONNECTING) 
                {
                    cFd.events |= POLLOUT;
                }
            }

            m_pollFds.push_back(cFd);
        }
        m_isPollDirty = false;
    }
    else 
    {
        usize idx = (!asClient && m_socket != SOCKET_INVALID) ? 1 : 0;
        for(auto& [_, pClient] : m_clients) 
        {
            if(idx >= m_pollFds.size()) 
                break;

            if(!pClient || pClient->status == SOCKET_CLIENT_CLOSE) 
            { 
                m_pollFds[idx].fd = SOCKET_INVALID;
                m_pollFds[idx].events = 0;
                idx++; 
                continue; 
            }
    
            m_pollFds[idx].events = 0;
    
        #ifdef SOCKET_USE_TLS
            if(pClient->pSsl && (pClient->sslWantsRead || pClient->sslWantsWrite)) 
            {
                if(pClient->sslWantsRead)  m_pollFds[idx].events |= POLLIN;
                if(pClient->sslWantsWrite) m_pollFds[idx].events |= POLLOUT;
            }
            else
        #endif
            {
                m_pollFds[idx].events |= POLLIN;
                if(pClient->sendQueue.GetDataSize() > 0 || pClient->status == SOCKET_CLIENT_CONNECTING) 
                {
                    m_pollFds[idx].events |= POLLOUT;
                }
            }

            idx++;
        }
    }

    if(m_pollFds.empty()) 
        return;

    int32 act = sys_poll(m_pollFds.data(), (unsigned long)m_pollFds.size(), 0);
    if(act <= 0) 
        return;

    usize startIdx = 0;
    if(!asClient && m_socket != SOCKET_INVALID) 
    {
        if(m_pollFds[0].revents & POLLIN) 
        {
            AcceptConnection();
        }
        startIdx = 1;
    }

    for(usize i = startIdx; i < m_pollFds.size(); ++i) 
    {
        socket_t fd = m_pollFds[i].fd;
        short revents = m_pollFds[i].revents;

        if(revents == 0) 
            continue;

        NetClient* pClient = GetClientByFD(fd);
        if(!pClient) 
            continue;

        if(revents & (POLLERR | POLLHUP)) {
            pClient->status = SOCKET_CLIENT_CLOSE;
            m_isPollDirty = true;
            continue;
        }

        if(revents & POLLIN) 
        {
            HandleReadIO(pClient);
        }

        if(revents & POLLOUT && pClient->status != SOCKET_CLIENT_CLOSE) 
        {
            HandleWriteIO(pClient);
        }
    }

    FlushClosedClients();
}

void NetSocket::FlushClosedClients()
{
    for(auto it = m_clients.begin(); it != m_clients.end();) 
    {
        NetClient* pClient = it->second;

        if(!pClient || pClient->status == SOCKET_CLIENT_CLOSE) 
        {
            if(pClient) 
            {
                socket_t fd = pClient->socket;

                if(fd >= 0 && fd < (socket_t)m_fdToClient.size()) {
                    m_fdToClient[fd] = nullptr;
                }

                if(fd != SOCKET_INVALID) {
                    CloseSocket(fd);
                }

#ifdef SOCKET_USE_TLS
                if(pClient->pSsl) {
                    SSL_shutdown(pClient->pSsl);
                    SSL_free(pClient->pSsl);
                    pClient->pSsl = nullptr;
                }
#endif
                m_events.Dispatch(SOCKET_EVENT_TYPE_DISCONNECT, pClient);
                
                SAFE_DELETE(pClient);
            }

            it = m_clients.erase(it);
            m_isPollDirty = true;
        }
        else 
        {
            ++it;
        }
    }
}

void NetSocket::HandleReadIO(NetClient* pClient)
{
    if(!pClient || pClient->status == SOCKET_CLIENT_CLOSE) 
        return;

    uint32 availableSpace = pClient->recvQueue.GetAvailableSpace();
    if(availableSpace == 0) {
        pClient->status = SOCKET_CLIENT_CLOSE;
        LOGGER_LOG_WARN("[NetSocket] RingBuffer overflow on fd: %d", pClient->socket);
        return;
    }

    uint32 readSize = availableSpace > SOCKET_MAX_BUFFER_SIZE ? SOCKET_MAX_BUFFER_SIZE : availableSpace;
    char buffer[SOCKET_MAX_BUFFER_SIZE];
    
    int32 val = 0;
    int32 sslErr = 0;

#ifdef SOCKET_USE_TLS
    if(pClient->pSsl) 
    {
        val = SSL_read(pClient->pSsl, buffer, readSize);
        if(val <= 0) 
        {
            sslErr = SSL_get_error(pClient->pSsl, val);
        }
    } 
    else
#endif
    {
#ifdef _WIN32
        val = recv(pClient->socket, buffer, readSize, 0);
#else
        val = recv(pClient->socket, buffer, readSize, MSG_DONTWAIT);
#endif
    }

    if(val > 0) 
    {
        pClient->recvQueue.Write(buffer, val);
        
#ifdef SOCKET_USE_TLS
        pClient->sslWantsRead = false;
        pClient->sslWantsWrite = false;
#endif

        m_events.Dispatch(SOCKET_EVENT_TYPE_RECEIVE, pClient);
    }
    else if(val == 0) 
    {
        pClient->status = SOCKET_CLIENT_CLOSE;
    }
    else 
    {
#ifdef SOCKET_USE_TLS
        if(pClient->pSsl) {
            if(sslErr == SSL_ERROR_WANT_READ) 
            { 
                pClient->sslWantsRead = true; 
                return; 
            }

            if(sslErr == SSL_ERROR_WANT_WRITE) 
            { 
                pClient->sslWantsWrite = true; 
                m_isPollDirty = true; 
                return; 
            }

            pClient->status = SOCKET_CLIENT_CLOSE; return;
        }
#endif
#ifdef _WIN32
        if(WSAGetLastError() != WSAEWOULDBLOCK)
        {
            pClient->status = SOCKET_CLIENT_CLOSE;
        }
#else
        if(errno != EAGAIN && errno != EWOULDBLOCK)
        {
            pClient->status = SOCKET_CLIENT_CLOSE;
        }
#endif
    }
}

void NetSocket::HandleWriteIO(NetClient* pClient)
{
    if(!pClient || pClient->status == SOCKET_CLIENT_CLOSE) 
        return;

    if(pClient->status == SOCKET_CLIENT_CONNECTING) 
    {
        int error = 0;

    #ifdef _WIN32
            int len = sizeof(error);
            if(getsockopt(pClient->socket, SOL_SOCKET, SO_ERROR, (char*)&error, &len) != 0 || error != 0) {
    #else
            socklen_t len = sizeof(error);
            if(getsockopt(pClient->socket, SOL_SOCKET, SO_ERROR, &error, &len) != 0 || error != 0) {
    #endif
            LOGGER_LOG_ERROR("[NetSocket] Non-block connect failed on fd: %d, Error: %d", pClient->socket, error);
            pClient->status = SOCKET_CLIENT_CLOSE;
            m_isPollDirty = true;
            return;
        }

#ifdef SOCKET_USE_TLS
        if(pClient->pSsl) 
        {
            int32 ret = SSL_do_handshake(pClient->pSsl);
            if(ret == 1) 
            {
                pClient->status = SOCKET_CLIENT_CONNECTED;
                pClient->sslWantsRead = false;
                pClient->sslWantsWrite = false;
                m_isPollDirty = true;
                m_events.Dispatch(SOCKET_EVENT_TYPE_CONNECT, pClient);
                return;
            }

            int32 sslErr = SSL_get_error(pClient->pSsl, ret);
            if(sslErr == SSL_ERROR_WANT_READ) 
            { 
                pClient->sslWantsRead = true; 
                pClient->sslWantsWrite = false; 
                m_isPollDirty = true; 
                return; 
            }

            if(sslErr == SSL_ERROR_WANT_WRITE) 
            { 
                pClient->sslWantsWrite = true; 
                pClient->sslWantsRead = false; 
                return; 
            }

            pClient->status = SOCKET_CLIENT_CLOSE;
            return;
        }
#endif
        pClient->status = SOCKET_CLIENT_CONNECTED;
        m_isPollDirty = true;
        m_events.Dispatch(SOCKET_EVENT_TYPE_CONNECT, pClient);
        return;
    }

    std::lock_guard<std::mutex> lock(pClient->sendMutex);
    uint32 dataSize = pClient->sendQueue.GetDataSize();
    if(dataSize == 0) 
        return;

    uint32 chunkSize = dataSize > SOCKET_MAX_BUFFER_SIZE ? SOCKET_MAX_BUFFER_SIZE : dataSize;
    char buffer[SOCKET_MAX_BUFFER_SIZE];

    pClient->sendQueue.Peek(buffer, chunkSize);

    int32 val = 0;
    int32 sslErr = 0;

#ifdef SOCKET_USE_TLS
    if(pClient->pSsl) {
        val = SSL_write(pClient->pSsl, buffer, chunkSize);
        if(val <= 0) 
        {
            sslErr = SSL_get_error(pClient->pSsl, val);
        }
    } 
    else
#endif
    {
#ifdef _WIN32
        val = send(pClient->socket, buffer, chunkSize, 0);
#else
        val = send(pClient->socket, buffer, chunkSize, MSG_DONTWAIT);
#endif
    }

    if(val > 0) 
    {
        pClient->sendQueue.Skip(val);
        
#ifdef SOCKET_USE_TLS
        pClient->sslWantsRead = false;
        pClient->sslWantsWrite = false;
#endif

    }
    else 
    {
#ifdef SOCKET_USE_TLS
        if(pClient->pSsl) {
            if(sslErr == SSL_ERROR_WANT_WRITE) 
            { 
                pClient->sslWantsWrite = true; 
                return; 
            }

            if(sslErr == SSL_ERROR_WANT_READ) 
            { 
                pClient->sslWantsRead = true; 
                m_isPollDirty = true; 
                return; 
            }
            
            pClient->status = SOCKET_CLIENT_CLOSE; return;
        }
#endif
#ifdef _WIN32
        if(WSAGetLastError() != WSAEWOULDBLOCK)
        {
            pClient->status = SOCKET_CLIENT_CLOSE;
        }
#else
        if(errno != EAGAIN && errno != EWOULDBLOCK)
        {
            pClient->status = SOCKET_CLIENT_CLOSE;
        }
#endif
    }
}

void NetSocket::AcceptConnection()
{
    sockaddr_in sockAddrClient;
#ifdef _WIN32
    int sockAddrCliLength = sizeof(sockAddrClient);
    socket_t socketClient = accept(m_socket, (SOCKADDR*)&sockAddrClient, &sockAddrCliLength);
#else
    socklen_t sockAddrCliLength = sizeof(sockAddrClient);
    socket_t socketClient = accept(m_socket, (sockaddr*)&sockAddrClient, &sockAddrCliLength);
#endif

    if(socketClient < 0) {
        return;
    }

    if(!MakeSocketNonBlocking(socketClient)) {
        CloseSocket(socketClient);
        return;
    }

#ifdef SOCKET_USE_TLS
    SSL* pSsl = SSL_new(m_pSslCtx);
    if(!pSsl) {
        CloseSocket(socketClient);
        return;
    }

    if(SSL_set_fd(pSsl, socketClient) != 1) {
        SSL_free(pSsl);
        CloseSocket(socketClient);
        return;
    }
    SSL_set_accept_state(pSsl);
#endif

    if(socketClient >= (socket_t)m_fdToClient.size()) 
    {
        //uhh
        m_fdToClient.resize(socketClient + 128, nullptr);
    }

    NetClient* pClient = new NetClient();
    pClient->socket = socketClient;
    pClient->connectionID = m_lastConnID++;
    pClient->pNetSocket = this;
    pClient->status = SOCKET_CLIENT_CONNECTING;
    pClient->ip = GetIPFromSocket(socketClient);

#ifdef SOCKET_USE_TLS
    pClient->pSsl = pSsl;
#endif

    m_fdToClient[socketClient] = pClient;
    m_isPollDirty = true;
    m_clients.insert_or_assign(pClient->connectionID, pClient);
}

void NetSocket::CloseClient(uint16 connectionID)
{
    auto it = m_clients.find(connectionID);
    if(it != m_clients.end()) 
    {
        NetClient* pClient = it->second;
        if(pClient) 
        {
            socket_t fd = pClient->socket;

            if(fd >= 0 && fd < (socket_t)m_fdToClient.size()) 
            {
                m_fdToClient[fd] = nullptr;
            }

            if(fd != SOCKET_INVALID) 
            {
                CloseSocket(fd);
            }

#ifdef SOCKET_USE_TLS
            if(pClient->pSsl) 
            {
                SSL_shutdown(pClient->pSsl);
                SSL_free(pClient->pSsl);
                pClient->pSsl = nullptr;
            }
#endif
            m_events.Dispatch(SOCKET_EVENT_TYPE_DISCONNECT, pClient);
            SAFE_DELETE(pClient);
        }

        m_clients.erase(it);
        m_isPollDirty = true;
    }
}

void NetSocket::CloseAllClients()
{
    while(!m_clients.empty()) 
    {
        CloseClient(m_clients.begin()->first);
    }

    m_clients.clear();
}

NetClient* NetSocket::GetClient(int16 connectionID)
{
    auto it = m_clients.find(connectionID);
    if(it == m_clients.end()) {
        return nullptr;
    }

    return it->second;
}

bool NetSocket::Send(NetClient* pClient, void* pData, uint32 size)
{
    if(!pClient || !pData || size == 0) {
        return false;
    }

    auto it = m_clients.find(pClient->connectionID);
    if(it == m_clients.end()) {
        return false;
    }

    if(pClient->status == SOCKET_CLIENT_CLOSE)
        return false;

    {
        std::lock_guard<std::mutex> lock(pClient->sendMutex);
        pClient->sendQueue.Write(pData, size);
    }

    return true;
}

NetClient* NetSocket::GetClientByFD(socket_t fd)
{
    if(fd == SOCKET_INVALID || fd < 0 || fd >= (socket_t)m_fdToClient.size())
        return nullptr;
    
    return m_fdToClient[fd];
}
