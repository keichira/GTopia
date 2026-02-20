#pragma once

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <errno.h>
#endif

#include "../Precompiled.h"
#include "NetClient.h"
#include "../Event/EventDispatcher.h"

bool MakeSocketNonBlocking(int32 fd);
void CloseSocket(int32 fd);

enum eSocketEventType
{
    SOCKET_EVENT_TYPE_CONNECT,
    SOCKET_EVENT_TYPE_RECEIVE,
    SOCKET_EVENT_TYPE_DISCONNECT
};

class NetSocket {
public:
    typedef EventDispatcher<eSocketEventType, NetClient*> SocketEventDispatcher;

public:
    NetSocket();
    ~NetSocket();

public:
    bool Init(const string& host, uint16 port, int32 backLog = 50);
    void Connect(const string& host, uint16 port);
    void Kill();

    void Update(bool asClient);
    void UpdateIO(const fd_set& rs, const fd_set& ws);
    void AcceptConnection();

    void CloseClient(uint16 connectionID);
    void CloseAllClients();
    NetClient* GetClient(int16 connectionID);

    bool Send(NetClient* pClient, uint8* pData, uint32 size);

    SocketEventDispatcher& GetEvents() { return m_events; }

private:
    int32 m_socket;
    int16 m_lastConnID;

    SocketEventDispatcher m_events;
    std::unordered_map<int16, NetClient*> m_clients;
};