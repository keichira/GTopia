#include "NetSocket.h"
#include "../IO/Log.h"

bool MakeSocketNonBlocking(int32 fd)
{
#ifdef _WIN32
    u_long mode = 1;
    return ioctlsocket(fd, FIONBIO, &mode) == 0;
#else
    int32 flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        return false;
    }
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
#endif
}

void CloseSocket(int32 fd)
{
#ifdef _WIN32
    closesocket(fd);
#else
    close(fd);
#endif
}

NetSocket::NetSocket()
: m_socket(-1), m_lastConnID(0)
{
#ifdef SOCKET_USE_TLS
    m_pSslCtx = nullptr;
#endif
}

NetSocket::~NetSocket()
{
    Kill();
}

bool NetSocket::Init(const std::string& host, uint16 port, int32 backLog)
{
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        LOGGER_LOG_ERROR("WSAStartup failed.");
        return false;
    }
#endif

    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket < 0) {
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
    if(addr != -1) {
        sockAddr.sin_addr.s_addr = addr;
    }
    else {
        hostent* hNet = gethostbyname(host.c_str());
        if(!hNet) {
            return -2;
        }
    
        sockAddr.sin_addr = *(in_addr*)hNet->h_addr;
    }

    int32 socketCli = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
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

    SSL_set_tlsext_host_name(pSsl, host.c_str()); // ??
    SSL_set_fd(pSsl, socketCli);

    int32 sslRes = SSL_connect(pSsl);
    if(sslRes <= 0) {
        LOGGER_LOG_ERROR("fail ssl connect");
        return -1;
    }

#endif

    NetClient* pClient = new NetClient();
    pClient->connectionID = m_lastConnID++;
    pClient->status = result == 0 ? SOCKET_CLIENT_CONNECTED : SOCKET_CLIENT_CONNECTING;
    pClient->pNetSocket = this;
    pClient->socket = socketCli;

#ifdef SOCKET_USE_TLS
    pClient->socket = SSL_get_fd(pSsl);
    pClient->pSsl = pSsl;
#endif

    m_clients.insert_or_assign(pClient->connectionID, pClient);
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
    fd_set rs, ws;
    FD_ZERO(&rs);
    FD_ZERO(&ws);

    int32 maxFD = -1;
    if(!asClient) {
        if(m_socket < 0) {
            return;
        }

        maxFD = m_socket;
    }

    FD_SET(m_socket, &rs);

    for(auto it = m_clients.begin(); it != m_clients.end();) {
        NetClient* pClient = it->second;

        if(!pClient) {
            it = m_clients.erase(it);
            continue;
        }

        if(pClient->status == SOCKET_CLIENT_CLOSE || pClient->socket < 0) {
            CloseClient(pClient->connectionID);
            it = m_clients.erase(it);
            continue;
        }

#ifdef SOCKET_USE_TLS
        int32 fd = SSL_get_fd(pClient->pSsl);
#else
        int32 fd = pClient->socket;
#endif

        FD_SET(fd, &rs);
        if (pClient->status == SOCKET_CLIENT_CONNECTING || pClient->sendQueue.GetDataSize() != 0) {
            FD_SET(fd, &ws);
        }
        if (fd > maxFD) maxFD = fd;

        ++it;
    }

    timeval timeout = {0, 0};
    int32 act = select(maxFD + 1, &rs, &ws, nullptr, &timeout);
    if (act == 0) {
        return;
    }
    else if(act < -1) {
        // manage errno
    }

    if (m_socket >= 0 && FD_ISSET(m_socket, &rs) && !asClient) {
        AcceptConnection();
    }

    UpdateIO(rs, ws);
}

void NetSocket::UpdateIO(const fd_set& rs, const fd_set& ws)
{
    for(auto it = m_clients.begin(); it != m_clients.end();) {
        NetClient* pClient = it->second;

        if(!pClient) {
            it = m_clients.erase(it);
            continue;
        }

        if(pClient->status != SOCKET_CLIENT_CONNECTING && FD_ISSET(pClient->socket, &rs)) {
            char buffer[SOCKET_MAX_BUFFER_SIZE];

    #ifdef _WIN32
        int32 flags = 0;
    #else
        int32 flags = MSG_DONTWAIT;
    #endif

    #ifdef SOCKET_USE_TLS
            int32 val = SSL_read(pClient->pSsl, buffer, sizeof(buffer));
    #else
            int32 val = recv(pClient->socket, buffer, sizeof(buffer), flags);
    #endif

            if(val > 0) {
                if(pClient->recvQueue.GetAvailableSpace() < val) {
                    pClient->status = SOCKET_CLIENT_CLOSE;
                    LOGGER_LOG_WARN("Overflow on netsocket while writing recv data fd: %d", pClient->socket);
                }
                else {
                    pClient->mutex.lock();
                    uint32 written = pClient->recvQueue.Write(&buffer, val);
                    pClient->mutex.unlock();
    
                    m_events.Dispatch(SOCKET_EVENT_TYPE_RECEIVE, pClient);
                }
            }
            else if(val == 0) {
                pClient->status = SOCKET_CLIENT_CLOSE;
            }
            else {
            #ifdef _WIN32
                if(WSAGetLastError() != WSAEWOULDBLOCK) {
                    pClient->status = SOCKET_CLIENT_CLOSE;
                }
            #else
                if(errno != EAGAIN && errno != EWOULDBLOCK) {
                    pClient->status = SOCKET_CLIENT_CLOSE;
                }
            #endif
            }
        }

        if(pClient->status != SOCKET_CLIENT_CLOSE && FD_ISSET(pClient->socket, &ws)) {
            if(pClient->status == SOCKET_CLIENT_CONNECTING) {
                int error = 0;
    #ifdef _WIN32
            int32 len = sizeof(error);
    #else
            uint32 len = sizeof(error);
    #endif

                if (getsockopt(pClient->socket, SOL_SOCKET, SO_ERROR, &error, &len) == 0) {
                    if (error == 0) {
            #ifdef SOCKET_USE_TLS
                        int32 sslRes = SSL_connect(pClient->pSsl);

                        if(sslRes == 1) {
                            pClient->status = SOCKET_CLIENT_CONNECTED;
                            m_events.Dispatch(SOCKET_EVENT_TYPE_CONNECT, pClient);
                        }
                        else {
                            int32 sslErr = SSL_get_error(pClient->pSsl, sslRes);
                            if(sslErr != SSL_ERROR_WANT_READ || sslErr != SSL_ERROR_WANT_WRITE) {
                                pClient->status = SOCKET_CLIENT_CLOSE;
                            }
                        }
            #else
                        pClient->status = SOCKET_CLIENT_CONNECTED;
                        m_events.Dispatch(SOCKET_EVENT_TYPE_CONNECT, pClient);
            #endif
                    }
                    else {
                        LOGGER_LOG_ERROR("Failed to connect socket error %d", error)
                        pClient->status = SOCKET_CLIENT_CLOSE;
                    }
                }
            }
            else if(pClient->sendQueue.GetDataSize() > 0) {
                std::lock_guard<std::mutex> lock(pClient->mutex);

                while(pClient->sendQueue.GetDataSize() > 0) {
                    uint32 sendSize = pClient->sendQueue.GetDataSize();
                    if(sendSize > SOCKET_MAX_BUFFER_SIZE) {
                        sendSize = SOCKET_MAX_BUFFER_SIZE;
                    }

                    char buffer[SOCKET_MAX_BUFFER_SIZE];
                    uint32 readSize = pClient->sendQueue.Read(&buffer, sendSize);

                    if(readSize <= 0) {
                        break;
                    }

        #ifdef SOCKET_USE_TLS
                    uint32 sentSize = SSL_write(pClient->pSsl, buffer, readSize);
        #else
                    uint32 sentSize = send(pClient->socket, buffer, readSize, MSG_DONTWAIT);
        #endif

                    if(sentSize < readSize) {
                        pClient->sendQueue.Write(&buffer[sentSize], readSize - sentSize);
                        break;
                    }

                    //EAGAIN EWOULDBLOCK
                }
            }
        }

        if(pClient->status == SOCKET_CLIENT_CLOSE) {
            CloseClient(pClient->connectionID);
            it = m_clients.erase(it);
            continue;
        }
        else ++it;
    }
}

void NetSocket::AcceptConnection()
{
    sockaddr_in sockAddrClient;
#ifdef _WIN32
    int32 sockAddrCliLength = sizeof(sockAddrClient);
    SOCKET socketClient = accept(m_socket, (SOCKADDR*)&sockAddrClient, &sockAddrCliLength);
#else
    socklen_t sockAddrCliLength = sizeof(sockAddrClient);
    int32 socketClient = accept(m_socket, (sockaddr*)&sockAddrClient, &sockAddrCliLength);
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

    if(SSL_accept(pSsl) != 1) {
        SSL_free(pSsl);
        CloseSocket(socketClient);
        return;
    }
#endif

    NetClient* pClient = new NetClient();
    pClient->socket = socketClient;
    pClient->connectionID = m_lastConnID++;
    pClient->pNetSocket = this;

#ifdef SOCKET_USE_TLS
    pClient->pSsl = pSsl;
#endif

    m_clients.insert_or_assign(pClient->connectionID, pClient);
}

void NetSocket::CloseClient(uint16 connectionID)
{
    auto it = m_clients.find(connectionID);

    if(it != m_clients.end()) {
        NetClient* pClient = it->second;
        if(!pClient) {
            return;
        }

        CloseSocket(pClient->socket);

#ifdef SOCKET_USE_TLS
        if(pClient->pSsl) {
            SSL_get_shutdown(pClient->pSsl);
            SSL_free(pClient->pSsl);
        }
#endif

        m_events.Dispatch(SOCKET_EVENT_TYPE_DISCONNECT, pClient);
        SAFE_DELETE(pClient);
    }
}

void NetSocket::CloseAllClients()
{
    for(auto& it : m_clients) {
        CloseClient(it.first);
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

    {
        std::lock_guard<std::mutex> lock(pClient->mutex);
        pClient->sendQueue.Write(pData, size);
    }

    return true;
}
