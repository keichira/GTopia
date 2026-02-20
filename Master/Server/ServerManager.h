#pragma once

#include "Precompiled.h"
#include "Server/ServerBroadwayBase.h"
#include "Packet/NetPacket.h"

struct NetClientInfo
{
    uint64 lastHeartbeatTime = 0;
    string authKey = "";
    bool authed = false;
};

struct ServerInfo
{
    uint16 serverID = 0;
    uint32 ping = 0;
    uint32 playerCount = 0;
    uint32 worldCount = 0;
    int32 socketConnID = -1;
};

class ServerManager : public ServerBroadwayBase {
public:
    typedef EventDispatcher<eTCPPacketType, NetClient*, VariantVector> TCPEventDispatcher;

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

public:
    void AddServer(uint16 serverID, NetClient* pClient);
    void RemoveServer(uint16 serverID);

private:
    std::unordered_map<uint16, ServerInfo*> m_servers;
};

ServerManager* GetServerManager();