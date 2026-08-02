#pragma once

#include "Event/EventDispatcher.h"
#include "Packet/GamePacket.h"
#include "Server/ServerBroadwayBase.h"

class MasterBroadway : public ServerBroadwayBase
{
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
    bool Connect(const string& host, uint16 port, uint8 retryCount,
                 const volatile sig_atomic_t* shutdownFlag = nullptr);
    bool ConnectAndAuth(const string& host, uint16 port, uint8 maxConnectAttempts,
                        const volatile sig_atomic_t* shutdownFlag);

    eBroadwayAuthState GetAuthState() const { return m_authState; }
    void SetAuthState(eBroadwayAuthState state) { m_authState = state; }

private:
    template <void (*Function)(NetClient*, VariantVector&)> void RegisterEvent(eTCPPacketType packet)
    {
        m_events.Register(packet, Delegate<NetClient*, VariantVector&>::Create<Function>());
    }

private:
    EventDispatcher<int8, NetClient*, VariantVector&> m_events;
    NetClient* m_pNetClient;
    eBroadwayAuthState m_authState;
};

MasterBroadway* GetMasterBroadway();