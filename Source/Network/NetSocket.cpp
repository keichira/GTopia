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
: m_socket(-1)
{
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

    sockaddr_in sockAddr{};
    sockAddr.sin_family = AF_INET;
    sockAddr.sin_port = htons(port);

    if(host.empty()) {
        sockAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    }
    else {
        if(inet_addr(host.c_str()) == 0) {
            hostent* hNet = gethostbyname(host.c_str());
            if(!hNet) {
                return false;
            }
        
            sockAddr.sin_addr = *(in_addr*)hNet->h_addr;
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

void NetSocket::Connect(const string& host, uint16 port)
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
            return;
        }
    
        sockAddr.sin_addr = *(in_addr*)hNet->h_addr;
    }

    int32 socketCli = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    MakeSocketNonBlocking(socketCli);

    int32 result = connect(socketCli, (sockaddr*)&sockAddr, sizeof(sockAddr));

    NetClient* pClient = new NetClient();
    pClient->connectionID = m_lastConnID++;
    pClient->status = SOCKET_CLIENT_CONNECTING;
    pClient->pNetSocket = this;
    pClient->socket = socketCli;

    m_clients.insert_or_assign(pClient->connectionID, pClient);
}

void NetSocket::Kill()
{
    CloseAllClients();
    CloseSocket(m_socket);

    m_events.Kill();
}

void NetSocket::Update(bool asClient)
{
    if(m_socket < 0) {
        return;
    }

    fd_set rs, ws;
    FD_ZERO(&rs);
    FD_ZERO(&ws);

    int32 maxFD = m_socket;
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

        int32 fd = pClient->socket;
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

    if (FD_ISSET(m_socket, &rs) && !asClient) {
        AcceptConnection();
    }

    UpdateIO(rs, ws);
}

void NetSocket::UpdateIO(const fd_set& rs, const fd_set& ws)
{
    if(m_socket < 0) {
        return;
    }

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
            int32 val = recv(pClient->socket, buffer, sizeof(buffer), flags);

            if(val > 0) {
                pClient->mutex.lock();
                uint32 written = pClient->recvQueue.Write(&buffer, val);
                pClient->mutex.unlock();
                
                if(written < val) {
                    LOGGER_LOG_WARN("Overflow on netsocket while writing recv data fd: %d", pClient->socket);
                    // start dancing if this happens
                }

                m_events.Dispatch(SOCKET_EVENT_TYPE_RECEIVE, pClient);
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
                char error = 0;
    #ifdef _WIN32
            int32 len = sizeof(error);
    #else
            uint32 len = sizeof(error);
    #endif

                if (getsockopt(pClient->socket, SOL_SOCKET, SO_ERROR, &error, &len) == 0) {
                    if (error == 0) {
                        pClient->status = SOCKET_CLIENT_CONNECTED;
                        m_events.Dispatch(SOCKET_EVENT_TYPE_CONNECT, pClient);
                    }
                    else {
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

                    uint32 sentSize = send(pClient->socket, buffer, readSize, MSG_DONTWAIT);
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

    NetClient* pClient = new NetClient();
    pClient->socket = socketClient;
    pClient->connectionID = m_lastConnID++;
    pClient->pNetSocket = this;

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

        m_events.Dispatch(SOCKET_EVENT_TYPE_DISCONNECT, pClient);
        SAFE_DELETE(pClient);
    }
}

void NetSocket::CloseAllClients()
{
    for(auto& it : m_clients) {
        NetClient* pClient = it.second;

        CloseSocket(pClient->socket);
        SAFE_DELETE(pClient);
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

bool NetSocket::Send(NetClient* pClient, uint8* pData, uint32 size)
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
