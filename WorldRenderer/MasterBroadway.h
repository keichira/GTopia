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
    void OnClientDisconnect(NetClient* pClient) override;
    void RegisterEvents() override;
    void UpdateTCPLogic(uint64 maxTimeMS) override;

public:
    void SendHelloPacket();
    void SendAuthPacket(const string& authKey);
    void SendWorldRenderResult(bool succeed, uint32 userID, uint32 worldID);
    void SendServerKillPacket();
    bool IsConnected() { return m_pNetClient != nullptr; }
    bool Connect(const string& host, uint16 port, uint8 retryCount, const volatile sig_atomic_t* shutdownFlag = nullptr);

private:
    template<class T>
    void RegisterEvent(eTCPPacketType packet)
    {
        m_events.Register(
            packet,
            Delegate<NetClient*, VariantVector&>::Create<&T::Execute>()
        );
    }

private:
    EventDispatcher<int8, NetClient*, VariantVector&> m_events;
    NetClient* m_pNetClient;
};

MasterBroadway* GetMasterBroadway();