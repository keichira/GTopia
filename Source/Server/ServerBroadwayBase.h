#pragma once

#include "../Network/NetSocket.h"
#include <concurrentqueue.h>
#include <signal.h>

#define CONNECT_SOCKET_TIMEOUT_MS 5000

struct TCPPacketEvent
{
    NetClient* pClient;
    VariantVector data;
    uint64 reqTime;
};

class ServerBroadwayBase {
public:
    ServerBroadwayBase();
    virtual ~ServerBroadwayBase();

public:
    virtual bool Init(const string& host, uint16 port, int32 backLog = 50);
    virtual void Kill();
    virtual void OnClientConnect(NetClient* pClient);
    virtual void OnClientReceive(NetClient* pClient);
    virtual void OnClientDisconnect(NetClient* pClient);
    virtual void RegisterEvents();
    virtual void Update(bool asClient);
    virtual void UpdateTCPLogic(uint64 maxTimeMS);

public:
    bool Connect(const string& host, uint16 port, uint8 retryCount, const volatile sig_atomic_t* shutdownFlag = nullptr);
    bool IsConnected() const { return m_connected; }
    bool SendPacketRaw(VariantVector& data);

protected:
    NetSocket* m_pNetSocket;
    bool m_connected;
    NetClient* m_pNetClient;
    moodycamel::ConcurrentQueue<TCPPacketEvent> m_packetQueue;
};