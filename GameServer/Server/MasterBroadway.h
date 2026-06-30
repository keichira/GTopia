#pragma once

#include "Event/EventDispatcher.h"
#include "Packet/GamePacket.h"
#include "Server/ServerBroadwayBase.h"
#include "Utils/Timer.h"

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
    void SendCheckSessionPacket(int32 netID, uint32 userID, uint32 token, uint16 serverID);
    void SendRenderWorldRequest(uint32 userID, uint32 worldInstanceID);
    void SendWorldInitResult(bool succeed, uint32 worldInstanceID);
    void SendPlayerWorldJoin(uint32 playerUserID, const string& worldName, const string& doorID);
    void SendHeartBeat();
    void SendEndPlayerSession(uint32 userID);
    void SendServerKillPacket();
    void SendPlayerJoinedWorld(uint32 playerUserID, uint32 worldInstanceID);
    void SendPlayerLeftWorld(uint32 playerUserID, uint32 worldInstanceID);
    void SendPlayerPresenceSubscribe(const std::vector<uint32>& ids);
    void SendPlayerPresenceUnsubscribe(const std::vector<uint32>& ids);

    bool IsConnected() { return m_pNetClient != nullptr; }
    bool Connect(const string& host, uint16 port, uint8 retryCount, const volatile sig_atomic_t* shutdownFlag = nullptr);
    bool ConnectAndAuth(const string& host, uint16 port, uint8 maxConnectAttempts, const volatile sig_atomic_t* shutdownFlag);

    eBroadwayAuthState GetAuthState() const { return m_authState; }
    void SetAuthState(eBroadwayAuthState state) { m_authState = state; }

private:
    template <void (*Function)(NetClient*, VariantVector&)> void RegisterEvent(eTCPPacketType packet)
    {
        m_events.Register(packet, Delegate<NetClient*, VariantVector&>::Create<Function>());
    }

private:
    EventDispatcher<int8, NetClient*, VariantVector&> m_events;
    Timer m_lastHearthBeatSentTime;
    Timer m_lastHearthBeatRecvTime;
    eBroadwayAuthState m_authState;
    NetClient* m_pNetClient;
};

MasterBroadway* GetMasterBroadway();