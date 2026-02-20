#pragma once

#include "../Network/NetSocket.h"
#include <concurrentqueue.h>

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

protected:
    NetSocket* m_pNetSocket;
    moodycamel::ConcurrentQueue<TCPPacketEvent> m_packetQueue;
};