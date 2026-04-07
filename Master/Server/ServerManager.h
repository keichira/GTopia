#pragma once

#include "Precompiled.h"
#include "Server/ServerBroadwayBase.h"
#include "Packet/NetPacket.h"
#include "Utils/GameConfig.h"
#include "Utils/Timer.h"

struct NetServerInfo
{
    Timer lastHeartbeatTime;
    string authKey = "";
    bool authed = false;
    uint32 serverID = 0;
};

struct ServerInfo
{
    uint16 serverID = 0;
    uint32 playerCount = 0;
    uint32 worldCount = 0;
    int32 socketConnID = -1;
    string wanIP;
    uint16 port;
    eConfigServerType serverType;
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
    void AddServer(uint16 serverID, NetClient* pClient, int8 serverType);
    void RemoveServer(uint16 serverID);
    ServerInfo* GetBestGameServer();
    ServerInfo* GetBestRenderServer();
    bool HasAnyGameServer();

    ServerInfo* GetServerByID(uint16 serverID);
    bool SendPacketRaw(uint16 serverID, VariantVector& data);

    void SendWorldPlayerFailPacket(int32 playerNetID, uint32 serverID);
    void SendWorldPlayerSuccessPacket(int32 playerNetID, uint32 serverID, uint32 worldID, const string& serverIP, uint16 serverPort, uint32 serverIDForPacket);
    void SendWorldInitPacket(const string& worldName, uint32 serverID);
    void SendAuthPacket(bool succeed, uint32 serverID);
    void SendRenderResult(int32 result, uint32 playerUserID, const string& worldName, uint32 serverID);
    void SendRenderRequest(uint32 playerUserID, uint32 worldID, uint32 serverID);

    void SendPlayerSessionCheck(bool hasSession, int32 playerNetID, int16 connectionID);
    void SendHelloPacket(const string& authKey, int16 connectionID);

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
    std::unordered_map<uint16, ServerInfo*> m_servers;
    EventDispatcher<int8, NetClient*, VariantVector&> m_events;
};

ServerManager* GetServerManager();