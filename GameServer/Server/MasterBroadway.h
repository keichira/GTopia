#pragma once

#include "Server/ServerBroadwayBase.h"
#include "Event/EventDispatcher.h"
#include "Packet/GamePacket.h"

class MasterBroadway : public ServerBroadwayBase {
public:
    MasterBroadway();
    ~MasterBroadway();

public:
    static MasterBroadway* GetInstance()
    {
        static MasterBroadway instance;
        return &instance;
    }

public:
    void OnClientConnect(NetClient* pClient) override;
    void RegisterEvents() override;
    void UpdateTCPLogic(uint64 maxTimeMS) override;

public:
    void Connect(const string& host, uint16 port);
    void RefreshForConnect();

    void SendHelloPacket();
    void SendCheckSessionPacket(int32 netID, uint32 userID, uint32 token, uint16 serverID);

    bool IsConnected() const { return m_connected; }

private:
    NetClient* m_pClient;
    bool m_connected;
    EventDispatcher<int8, NetClient*, VariantVector&> m_events;
};

MasterBroadway* GetMasterBroadway();