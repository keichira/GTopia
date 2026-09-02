#pragma once

#include "Network/NetEntity.h"
#include "Packet/NetPacket.h"
#include "Precompiled.h"
#include "Server/ServerBroadwayBase.h"
#include "Utils/GameConfig.h"
#include "Utils/Timer.h"

class ServerInfo : public NetEntity
{
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

class ServerManager : public ServerBroadwayBase
{
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
    bool AddServer(ServerInfo* pServer, uint16 serverID, int8 serverType);
    void RemoveServer(uint16 serverID);
    ServerInfo* GetBestGameServer();
    ServerInfo* GetBestRenderServer();
    bool HasAnyGameServer();

    uint32 GetPlayerCount();

    void UpdateServers();

    ServerInfo* GetServerByID(uint16 serverID);

    void SendWorldPlayerFailPacket(ServerInfo* pServer, uint32 playerUserID, const string& message);
    void SendWorldPlayerSuccessPacket(ServerInfo* pServer, uint32 playerUserID, uint32 serverID, uint32 instanceID,
                                      const string& doorID, const string& serverIP, uint16 serverPort);
    void SendWorldInitPacket(ServerInfo* pServer, const string& worldName, uint32 instanceID, uint32 databaseID);
    void SendAuthPacket(ServerInfo* pServer, bool succeed);
    void SendRenderResult(ServerInfo* pServer, int32 result, uint32 playerUserID, uint32 databaseID);
    void SendRenderRequest(ServerInfo* pServer, uint32 playerUserID, uint32 worldID);
    void SendHeartBeat(ServerInfo* pServer, uint32 totalPlayer);
    void SendCommandSetRole(ServerInfo* pServer, uint32 userID, uint32 roleID);
    void SendPlayerSessionCheck(ServerInfo* pServer, bool hasSession, int32 playerNetID, uint32 worldInstanceID);
    void SendHelloPacket(ServerInfo* pServer, const string& authKey);

    void SendPlayerPresenceSnapshot(ServerInfo* pServer, const std::vector<PlayerPresencePacketElement>& elements);
    void SendPlayerPresenceUpdate(ServerInfo* pServer, const std::vector<PlayerPresencePacketElement>& elements);
    void SendWorldPresenceSnapshot(ServerInfo* pServer, const std::vector<WorldPresenceSnapshotElement>& elements);
    void SendWorldPresenceSnapshotToAll(const WorldPresenceSnapshotElement& snapshotInfo);
    void SendWorldPresenceUpdateToAll(const std::vector<WorldPresenceUpdateElement>& elements);
    void SendWorldPresenceRemoveToAll(const std::vector<WorldPresenceRemoveElement>& elements);

private:
    template <void (*Function)(NetClient*, TCPPacketHeader&, TCPPacketReader&)>
    void RegisterEvent(eTCPPacketType packet)
    {
        m_events.Register(packet, Delegate<NetClient*, TCPPacketHeader&, TCPPacketReader&>::Create<Function>());
    }

    template <typename T> void SendArray(ServerInfo* pServer, eTCPPacketType packetType, const std::vector<T>& elements)
    {
        if (!pServer || !pServer->pClient || elements.empty())
            return;

        TCPPacketWriter writer((uint16)packetType, (elements.size() * sizeof(T)) + sizeof(uint32));
        writer.WriteArray(elements);
        pServer->pClient->Send(writer);
    }

    template <typename T> void SendArrayToAll(eTCPPacketType packetType, const std::vector<T>& elements)
    {
        if (elements.empty() || m_servers.empty())
            return;

        TCPPacketWriter writer((uint16)packetType, (elements.size() * sizeof(T)) + sizeof(uint32));
        writer.WriteArray(elements);

        for (auto& [_, pServer] : m_servers)
        {
            if (!pServer || !pServer->pClient || pServer->serverType != CONFIG_SERVER_GAME)
                continue;

            pServer->pClient->Send(writer);
        }
    }

private:
    Timer m_lastServerUpdateTime;
    Timer m_lastHeartBeatTime;
    Timer m_lastPendingUpdateTime;

    std::unordered_map<uint32, ServerInfo*> m_pendingClients;
    std::unordered_map<uint16, ServerInfo*> m_servers;

    EventDispatcher<uint16, NetClient*, TCPPacketHeader&, TCPPacketReader&> m_events;
};

ServerManager* GetServerManager();