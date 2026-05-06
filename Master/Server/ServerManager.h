#pragma once

#include "Precompiled.h"
#include "Server/ServerBroadwayBase.h"
#include "Packet/NetPacket.h"
#include "Utils/GameConfig.h"
#include "Utils/Timer.h"
#include "Network/NetEntity.h"

class ServerInfo : public NetEntity {
public:
    ServerInfo(NetClient* pNetClient);

public:
    NetClient* pClient;
    uint16 serverID = 0;
    uint32 playerCount = 0;
    uint32 worldCount = 0;
    string wanIP;
    uint16 port;
    eConfigServerType serverType;

    Timer lastHeartbeatTime;
    string authKey = "";
    bool authed = false;
    bool deleteFlag = false;
};

class ServerManager : public ServerBroadwayBase {
public:
    ServerManager();
    ~ServerManager();

public:
    static ServerManager* GetInstance()
    {
        static ServerManager instance;
        return &instance;
    }

public:
    void OnClientConnect(NetClient* pClient) override;
    void OnClientDisconnect(NetClient* pClient) override;
    void UpdateTCPLogic(uint64 maxTimeMS) override;
    void Kill() override;
    void RegisterEvents() override;

public:
    void AddServer(ServerInfo* pServer, uint16 serverID, int8 serverType);
    void RemoveServer(uint16 serverID);
    ServerInfo* GetBestGameServer();
    ServerInfo* GetBestRenderServer();
    bool HasAnyGameServer();

    uint32 GetPlayerCount();

    void UpdateServers();

    ServerInfo* GetServerByID(uint16 serverID);

    void SendWorldPlayerFailPacket(ServerInfo* pServer, uint32 playerUserID, const string& message);
    void SendWorldPlayerSuccessPacket(ServerInfo* pServer, uint32 playerUserID, uint32 serverID, uint32 instanceID, const string& serverIP, uint16 serverPort);
    void SendWorldInitPacket(ServerInfo* pServer, const string& worldName, uint32 instanceID, uint32 databaseID);
    void SendAuthPacket(ServerInfo* pServer, bool succeed);
    void SendRenderResult(ServerInfo* pServer, int32 result, uint32 playerUserID, const string& worldName);
    void SendRenderRequest(ServerInfo* pServer, uint32 playerUserID, uint32 worldID);
    void SendHeartBeat(ServerInfo* pServer, uint32 totalPlayer);
    void SendCommandSetRole(ServerInfo* pServer, uint32 userID, uint32 roleID);
    void SendPlayerSessionCheck(ServerInfo* pServer, bool hasSession, int32 playerNetID, uint32 worldInstanceID);
    void SendHelloPacket(ServerInfo* pServer, const string& authKey);

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
    Timer m_lastServerUpdateTime;
    Timer m_lastHeartBeatTime;
    Timer m_lastPendingUpdateTime;

    std::unordered_map<uint32, ServerInfo*> m_pendingClients;
    std::unordered_map<uint16, ServerInfo*> m_servers;

    EventDispatcher<int8, NetClient*, VariantVector&> m_events;
};

ServerManager* GetServerManager();