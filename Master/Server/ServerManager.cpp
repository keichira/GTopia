#include "ServerManager.h"
#include "Utils/Timer.h"
#include "IO/Log.h"
#include "../Context.h"
#include "../Player/PlayerManager.h"
#include "../World/WorldManager.h"

#include "../Event/TCP/TCPEventHello.h"
#include "../Event/TCP/TCPEventAuth.h"
#include "../Event/TCP/TCPEventPlayerSession.h"
#include "../Event/TCP/TCPEventWorldInit.h"
#include "../Event/TCP/TCPEventRenderWorld.h"
#include "../Event/TCP/TCPEventWorldSendPlayer.h"
#include "../Event/TCP/TCPEventPlayerEndSession.h"
#include "../Event/TCP/TCPEventKillServer.h"
#include "../Event/TCP/TCPEventHeartBeat.h"

ServerInfo::ServerInfo(NetClient* pNetClient)
: NetEntity(ENTITY_TYPE_SERVER)
{
    pClient = pNetClient;
}

ServerManager::ServerManager()
{
}

ServerManager::~ServerManager()
{
    Kill();
}

void ServerManager::OnClientConnect(NetClient* pClient)
{
    if(!pClient)
        return;

    ServerInfo* pServerInfo = new ServerInfo(pClient);
    pClient->data = pServerInfo;

    m_pendingClients.insert_or_assign(pServerInfo->GetNetID(), pServerInfo);
}

void ServerManager::OnClientDisconnect(NetClient* pClient)
{
    if(!pClient) {
        return;
    }

    ServerInfo* pServerInfo = (ServerInfo*)pClient->data;
    pClient->data = nullptr;

    if(!pServerInfo)
        return;

    uint32 netID = pServerInfo->GetNetID();
    uint16 serverID = pServerInfo->serverID;

    if(serverID == 0) {
        m_pendingClients.erase(netID);
        SAFE_DELETE(pServerInfo);
        return;
    }

    RemoveServer(serverID);
}

void ServerManager::UpdateTCPLogic(uint64 maxTimeMS)
{
    Timer startTime;
    TCPPacketEvent event;

    while(m_packetQueue.try_dequeue(event)) {
        if(!event.pClient) {
            continue;
        }

        ServerInfo* pServerInfo = (ServerInfo*)event.pClient->data;
        if(!pServerInfo) {
            event.pClient->status = SOCKET_CLIENT_CLOSE;
            continue;
        }

        int8 type = event.data[0].GetINT();
        if(type != TCP_PACKET_HEARTBEAT) {
            LOGGER_LOG_DEBUG("GOT TCP PACKET %d", type);
        }

        switch(type) {
            case TCP_PACKET_HELLO: 
            case TCP_PACKET_AUTH: {
                m_events.Dispatch(type, event.pClient, event.data);
                break;
            }

            default: {
                if(!pServerInfo->authed) {
                    LOGGER_LOG_WARN("Client trying to access un-authorized packets?! CLOSING!");
                    event.pClient->status = SOCKET_CLIENT_CLOSE;
                    continue;
                }

                m_events.Dispatch(type, event.pClient, event.data);
            }
        }

        if(startTime.GetElapsedTime() >= maxTimeMS) {
            break;
        }
    }
}

void ServerManager::Kill()
{
    while(!m_servers.empty()) {
        RemoveServer(m_servers.begin()->first);
    }

    ServerBroadwayBase::Kill();
}

void ServerManager::RegisterEvents()
{
    ServerBroadwayBase::RegisterEvents();

    RegisterEvent<TCPEventHello>(TCP_PACKET_HELLO);
    RegisterEvent<TCPEventAuth>(TCP_PACKET_AUTH);
    RegisterEvent<TCPEventPlayerSession>(TCP_PACKET_PLAYER_CHECK_SESSION);
    RegisterEvent<TCPEventWorldInit>(TCP_PACKET_WORLD_INIT);
    RegisterEvent<TCPEventRenderWorld>(TCP_PACKET_RENDER_WORLD);
    RegisterEvent<TCPEventWorldSendPlayer>(TCP_PACKET_WORLD_SEND_PLAYER);
    RegisterEvent<TCPEventPlayerEndSession>(TCP_PACKET_PLAYER_END_SESSION);
    RegisterEvent<TCPEventKillServer>(TCP_PACKET_KILL_SERVER);
    RegisterEvent<TCPEventHeartBeat>(TCP_PACKET_HEARTBEAT);
}

ServerInfo* ServerManager::GetServerByID(uint16 serverID)
{
    auto it = m_servers.find(serverID);
    if(it == m_servers.end()) {
        return nullptr;
    }

    return it->second;
}

void ServerManager::SendWorldPlayerFailPacket(ServerInfo* pServer, uint32 playerUserID, const string& message)
{
    if(!pServer || !pServer->pClient) {
        return;
    }
    
    VariantVector data(4);

    data[0] = TCP_PACKET_WORLD_SEND_PLAYER;
    data[1] = TCP_RESULT_FAIL;
    data[2] = playerUserID;
    data[3] = message;

    pServer->pClient->Send(data);
}

void ServerManager::SendWorldPlayerSuccessPacket(ServerInfo* pServer, uint32 playerUserID, uint32 serverID, uint32 instanceID, const string& serverIP, uint16 serverPort)
{
    if(!pServer || !pServer->pClient) {
        return;
    }

    VariantVector data(7);

    data[0] = TCP_PACKET_WORLD_SEND_PLAYER;
    data[1] = TCP_RESULT_OK;   
    data[2] = playerUserID;
    data[3] = serverID;
    data[4] = instanceID;
    data[5] = serverIP;
    data[6] = (uint32)serverPort;

    pServer->pClient->Send(data);
}

void ServerManager::SendWorldInitPacket(ServerInfo* pServer, const string& worldName, uint32 instanceID, uint32 databaseID)
{
    if(!pServer || !pServer->pClient)
        return;

    VariantVector data(4);

    data[0] = TCP_PACKET_WORLD_INIT;
    data[1] = worldName;
    data[2] = instanceID;
    data[3] = databaseID;

    pServer->pClient->Send(data);
}

void ServerManager::SendAuthPacket(ServerInfo* pServer, bool succeed)
{
    if(!pServer || !pServer->pClient) {
        return;
    }

    VariantVector data(2);

    data[0] = TCP_PACKET_AUTH;
    data[1] = succeed;

    pServer->pClient->Send(data);
}

void ServerManager::SendRenderResult(ServerInfo* pServer, int32 result, uint32 playerUserID, const string& worldName)
{
    if(!pServer || !pServer->pClient) {
        return;
    }

    VariantVector data(5);

    data[0] = TCP_PACKET_RENDER_WORLD;
    data[1] = TCP_RENDER_RESULT;
    data[2] = result;
    data[3] = playerUserID;
    data[4] = worldName;

    pServer->pClient->Send(data);
}

void ServerManager::SendRenderRequest(ServerInfo* pServer, uint32 playerUserID, uint32 worldID)
{
    if(!pServer || !pServer->pClient) {
        return;
    }

    VariantVector data(4);

    data[0] = TCP_PACKET_RENDER_WORLD;
    data[1] = TCP_RENDER_REQUEST;
    data[2] = playerUserID;
    data[3] = worldID;

    pServer->pClient->Send(data);
}

void ServerManager::SendHeartBeat(ServerInfo* pServer, uint32 totalPlayer)
{
    if(!pServer || !pServer->pClient) {
        return;
    }

    VariantVector data(4);

    data[0] = TCP_PACKET_HEARTBEAT;
    data[1] = totalPlayer;

    pServer->pClient->Send(data);
}

void ServerManager::SendCommandSetRole(ServerInfo* pServer, uint32 userID, uint32 roleID)
{
    if(!pServer || !pServer->pClient) {
        return;
    }

    VariantVector data(4);
    data[0] = TCP_PACKET_ADMIN_COMMAND;
    data[1] = TCP_COMMAND_SETROLE;
    data[2] = userID;
    data[3] = roleID;

    pServer->pClient->Send(data);
}

void ServerManager::SendPlayerSessionCheck(ServerInfo* pServer, bool hasSession, int32 playerNetID, uint32 worldInstanceID)
{
    if(!pServer || !pServer->pClient) {
        return;
    }

    VariantVector data(4);

    data[0] = TCP_PACKET_PLAYER_CHECK_SESSION;
    data[1] = playerNetID;
    data[2] = hasSession;
    data[3] = worldInstanceID;

    pServer->pClient->Send(data);
}

void ServerManager::SendHelloPacket(ServerInfo* pServer, const string& authKey)
{
    if(!pServer || !pServer->pClient) {
        return;
    }

    VariantVector data(2);
    
    data[0] = TCP_PACKET_HELLO;
    data[1] = authKey;

    pServer->pClient->Send(data);
}

void ServerManager::AddServer(ServerInfo* pServer, uint16 serverID, int8 serverType)
{
    if(!pServer || !pServer->pClient) {
        return;
    }

    if(serverType != CONFIG_SERVER_GAME && serverType != CONFIG_SERVER_RENDERER) {
        pServer->pClient->status = SOCKET_CLIENT_CLOSE;
        LOGGER_LOG_WARN("Unknown server %d type %d, closing", serverID, serverType);
        return;
    }

    auto it = m_servers.find(serverID);
    if(serverID == 0|| it != m_servers.end()) {
        pServer->pClient->status = SOCKET_CLIENT_CLOSE;
        LOGGER_LOG_ERROR("Server %d already exists but we tried to add it again??", serverID);
        return;
    }

    auto serverNetInfo = GetContext()->GetGameConfig()->servers[serverID];
    if(serverNetInfo.serverType != serverType || serverNetInfo.lanIP.empty() || serverNetInfo.wanIP.empty()) {
        pServer->pClient->status = SOCKET_CLIENT_CLOSE;
        return;
    }

    string serverTypeStr = "game";
    if(serverType == CONFIG_SERVER_RENDERER) serverTypeStr = "renderer";

    LOGGER_LOG_INFO("Server %d (%s) added to cache %s:%d", serverID, serverTypeStr.c_str(), serverNetInfo.wanIP.c_str(), serverNetInfo.udpPort);
    pServer->serverID = serverID;
    pServer->wanIP = serverNetInfo.wanIP;
    pServer->port = serverNetInfo.udpPort;
    pServer->serverType = (eConfigServerType)serverType;
    pServer->authed = true;
    pServer->lastHeartbeatTime.Reset();

    m_pendingClients.erase(pServer->GetNetID());
    m_servers.insert_or_assign(pServer->serverID, pServer);
}

void ServerManager::RemoveServer(uint16 serverID)
{
    auto it = m_servers.find(serverID);
    if (it == m_servers.end())
        return;

    ServerInfo* pServer = it->second;
    if (!pServer)
    {
        m_servers.erase(it);
        return;
    }

    if (pServer->pClient)
    {
        pServer->pClient->data = nullptr;
        pServer->pClient->status = SOCKET_CLIENT_CLOSE;
    }

    GetPlayerManager()->EndSessionsByServer(serverID);
    GetWorldManager()->EndSessionsByServerID(serverID);

    m_servers.erase(it);
    SAFE_DELETE(pServer);
}

ServerInfo* ServerManager::GetBestGameServer()
{
    ServerInfo* pBestServer = nullptr;
    float bestScore = 999999999.0f;

    for(auto& [_, pServer] : m_servers) {
        if(!pServer || pServer->serverType != CONFIG_SERVER_GAME) {
            continue;
        }

        float score = 1.0f * pServer->playerCount / (pServer->worldCount + 1);
        if(score < bestScore) {
            bestScore = score;
            pBestServer = pServer;
        }
    }

    return pBestServer;
}

ServerInfo* ServerManager::GetBestRenderServer()
{
    for(auto& [_, pServer] : m_servers) {
        if(!pServer || pServer->serverType != CONFIG_SERVER_RENDERER) {
            continue;
        }

        return pServer;
    }

    return nullptr;
}

bool ServerManager::HasAnyGameServer()
{
    for(auto& [_, pServer] : m_servers) {
        if(!pServer || pServer->serverType != CONFIG_SERVER_GAME) {
            continue;
        }

        return true;
    }

    return false;
}

uint32 ServerManager::GetPlayerCount()
{
    uint32 playerCount = 0;
    
    for(auto& [_, pServer] : m_servers) {
        if(!pServer) {
            continue;
        }

        playerCount += pServer->playerCount;
    }

    return playerCount;
}

void ServerManager::UpdateServers()
{
    if(m_lastServerUpdateTime.GetElapsedTime() < 1000)
        return;

    if(m_lastHeartBeatTime.GetElapsedTime() >= 5000)
    {
        PlayerManager* pPlayerMgr = GetPlayerManager();

        for(auto it = m_servers.begin(); it != m_servers.end();)
        {
            ServerInfo* pServer = it->second;

            if(!pServer)
            {
                it = m_servers.erase(it);
                continue;
            }

            if(!pServer->pClient)
            {
                SAFE_DELETE(pServer);
                it = m_servers.erase(it);
                continue;
            }

            if(pServer->lastHeartbeatTime.GetElapsedTime() >= 30 * 1000)
            {
                pServer->pClient->status = SOCKET_CLIENT_CLOSE;
                ++it;
                continue;
            }

            SendHeartBeat(pServer, pPlayerMgr->GetTotalPlayerCount());
            ++it;
        }

        m_lastHeartBeatTime.Reset();
    }

    if(m_lastPendingUpdateTime.GetElapsedTime() >= 2000)
    {
        for(auto it = m_pendingClients.begin(); it != m_pendingClients.end();)
        {
            ServerInfo* pServer = it->second;

            if(!pServer)
            {
                it = m_pendingClients.erase(it);
                continue;
            }

            if(!pServer->pClient)
            {
                SAFE_DELETE(pServer);
                it = m_pendingClients.erase(it);
                continue;
            }

            if(pServer->lastHeartbeatTime.GetElapsedTime() >= 60 * 1000)
            {
                pServer->pClient->status = SOCKET_CLIENT_CLOSE;
                pServer->pClient->data = nullptr;
                SAFE_DELETE(pServer);
                it = m_pendingClients.erase(it);
                continue;
            }

            ++it;
        }

        m_lastPendingUpdateTime.Reset();
    }

    m_lastServerUpdateTime.Reset();
}


ServerManager* GetServerManager() { return ServerManager::GetInstance(); }
