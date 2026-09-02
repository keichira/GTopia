#include "ServerManager.h"
#include "../Context.h"
#include "../Player/PlayerManager.h"
#include "../Server/GamePresenceManager.h"
#include "../World/WorldManager.h"
#include "IO/Log.h"
#include "Utils/Timer.h"

#include "../Event/TCP/TCPEvent_Player.h"
#include "../Event/TCP/TCPEvent_Server.h"
#include "../Event/TCP/TCPEvent_World.h"

ServerInfo::ServerInfo(NetClient* pNetClient) : NetEntity(ENTITY_TYPE_SERVER)
{
    pClient = pNetClient;
}

ServerManager::ServerManager() {}

ServerManager::~ServerManager()
{
    Kill();
}

void ServerManager::OnClientConnect(NetClient* pClient)
{
    if (!pClient)
        return;

    ServerInfo* pServerInfo = new ServerInfo(pClient);
    pClient->data = pServerInfo;

    m_pendingClients.insert_or_assign(pServerInfo->GetNetID(), pServerInfo);
}

void ServerManager::OnClientDisconnect(NetClient* pClient)
{
    if (!pClient)
    {
        return;
    }

    ServerInfo* pServerInfo = (ServerInfo*)pClient->data;
    pClient->data = nullptr;

    if (!pServerInfo)
        return;

    uint32 netID = pServerInfo->GetNetID();
    uint16 serverID = pServerInfo->serverID;

    if (serverID == 0)
    {
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

    while (m_packetQueue.try_dequeue(event))
    {
        if (!event.pClient || event.pClient->status == SOCKET_CLIENT_CLOSE)
            continue;

        ServerInfo* pServerInfo = (ServerInfo*)(event.pClient->data);
        if (!pServerInfo)
        {
            event.pClient->status = SOCKET_CLIENT_CLOSE;
            continue;
        }

        uint16 packetID = event.header.packetID;

        if (packetID != TCP_PACKET_HEARTBEAT)
        {
            LOGGER_LOG_DEBUG("Received TCP Packet %d (Size: %u)", packetID, event.header.bodySize);
        }

        if (packetID != TCP_PACKET_HELLO && packetID != TCP_PACKET_AUTH)
        {
            if (!pServerInfo->authed)
            {
                LOGGER_LOG_WARN("Client trying to access un-authorized packets?! CLOSING!");
                event.pClient->status = SOCKET_CLIENT_CLOSE;
                continue;
            }
        }

        TCPPacketReader reader(event.payload.data(), (uint32)(event.payload.size()));

        if (packetID == TCP_PACKET_PLAYER_SUBSCRIBE || packetID == TCP_PACKET_PLAYER_UNSUBSCRIBE ||
            packetID == TCP_PACKET_WORLD_UPDATE || packetID == TCP_PACKET_WORLD_REMOVE)
        {
            GetGamePresenceManager()->OnTCPPacket(event.pClient, packetID, reader);
        }
        else
        {
            m_events.Dispatch(packetID, event.pClient, event.header, reader);
        }

        if (startTime.GetElapsedTime() >= maxTimeMS)
            break;
    }
}

void ServerManager::Kill()
{
    while (!m_servers.empty())
    {
        RemoveServer(m_servers.begin()->first);
    }

    ServerBroadwayBase::Kill();
}

void ServerManager::RegisterEvents()
{
    ServerBroadwayBase::RegisterEvents();

    RegisterEvent<TCPEvent_Hello>(TCP_PACKET_HELLO);
    RegisterEvent<TCPEvent_Auth>(TCP_PACKET_AUTH);
    RegisterEvent<TCPEvent_PlayerCheckSession>(TCP_PACKET_PLAYER_CHECK_SESSION);
    RegisterEvent<TCPEvent_WorldInit>(TCP_PACKET_WORLD_INIT);
    RegisterEvent<TCPEvent_RenderWorld>(TCP_PACKET_RENDER_WORLD);
    RegisterEvent<TCPEvent_WorldSendPlayer>(TCP_PACKET_WORLD_SEND_PLAYER);
    RegisterEvent<TCPEvent_PlayerEndSession>(TCP_PACKET_PLAYER_END_SESSION);
    RegisterEvent<TCPEvent_KillServer>(TCP_PACKET_KILL_SERVER);
    RegisterEvent<TCPEvent_HeartBeat>(TCP_PACKET_HEARTBEAT);
    RegisterEvent<TCPEvent_WorldPlayerSession>(TCP_PACKET_WORLD_PLAYER_SESSION);
}

ServerInfo* ServerManager::GetServerByID(uint16 serverID)
{
    auto it = m_servers.find(serverID);
    if (it == m_servers.end())
        return nullptr;

    return it->second;
}

void ServerManager::SendWorldPlayerFailPacket(ServerInfo* pServer, uint32 playerUserID, const string& message)
{
    if (!pServer || !pServer->pClient)
        return;

    TCPPacketWriter writer(TCP_PACKET_WORLD_SEND_PLAYER);
    writer.Write<int32>(TCP_RESULT_FAIL);
    writer.Write<uint32>(playerUserID);
    writer.WriteString(message);

    pServer->pClient->Send(writer);
}

void ServerManager::SendWorldPlayerSuccessPacket(ServerInfo* pServer, uint32 playerUserID, uint32 serverID,
                                                 uint32 instanceID, const string& doorID, const string& serverIP,
                                                 uint16 serverPort)
{
    if (!pServer || !pServer->pClient)
        return;

    TCPPacketWriter writer(TCP_PACKET_WORLD_SEND_PLAYER, 256);
    writer.Write<int32>(TCP_RESULT_OK);
    writer.Write(playerUserID);
    writer.Write(serverID);
    writer.Write(instanceID);
    writer.WriteString(doorID);
    writer.WriteString(serverIP);
    writer.Write<uint32>(serverPort);

    pServer->pClient->Send(writer);
}

void ServerManager::SendWorldInitPacket(ServerInfo* pServer, const string& worldName, uint32 instanceID,
                                        uint32 databaseID)
{
    if (!pServer || !pServer->pClient)
        return;

    TCPPacketWriter writer(TCP_PACKET_WORLD_INIT);
    writer.WriteString(worldName);
    writer.Write(instanceID);
    writer.Write(databaseID);
    writer.Write(pServer->serverID);

    pServer->pClient->Send(writer);
}

void ServerManager::SendAuthPacket(ServerInfo* pServer, bool succeed)
{
    if (!pServer || !pServer->pClient)
        return;

    TCPPacketWriter writer(TCP_PACKET_AUTH);
    writer.Write<uint8>(succeed ? 1 : 0);

    pServer->pClient->Send(writer);
}

void ServerManager::SendRenderResult(ServerInfo* pServer, int32 result, uint32 playerUserID, uint32 databaseID)
{
    if (!pServer || !pServer->pClient)
        return;

    TCPPacketWriter writer(TCP_PACKET_RENDER_WORLD);
    writer.Write<int32>(TCP_RENDER_RESULT);
    writer.Write(result);
    writer.Write(playerUserID);
    writer.Write(databaseID);

    pServer->pClient->Send(writer);
}

void ServerManager::SendRenderRequest(ServerInfo* pServer, uint32 playerUserID, uint32 worldID)
{
    if (!pServer || !pServer->pClient)
        return;

    TCPPacketWriter writer(TCP_PACKET_RENDER_WORLD);
    writer.Write<int32>(TCP_RENDER_REQUEST);
    writer.Write(playerUserID);
    writer.Write(worldID);

    pServer->pClient->Send(writer);
}

void ServerManager::SendHeartBeat(ServerInfo* pServer, uint32 totalPlayer)
{
    if (!pServer || !pServer->pClient)
        return;

    TCPPacketWriter writer(TCP_PACKET_HEARTBEAT);
    writer.Write(totalPlayer);

    pServer->pClient->Send(writer);
}

void ServerManager::SendCommandSetRole(ServerInfo* pServer, uint32 userID, uint32 roleID)
{
    if (!pServer || !pServer->pClient)
        return;

    TCPPacketWriter writer(TCP_PACKET_COMMAND);
    writer.Write<int32>(TCP_COMMAND_SETROLE);
    writer.Write(userID);
    writer.Write(roleID);

    pServer->pClient->Send(writer);
}

void ServerManager::SendPlayerSessionCheck(ServerInfo* pServer, bool hasSession, int32 playerNetID,
                                           uint32 worldInstanceID)
{
    if (!pServer || !pServer->pClient)
        return;

    TCPPacketWriter writer(TCP_PACKET_PLAYER_CHECK_SESSION);
    writer.Write(playerNetID);
    writer.Write<uint8>(hasSession ? 1 : 0);
    writer.Write(worldInstanceID);

    pServer->pClient->Send(writer);
}

void ServerManager::SendHelloPacket(ServerInfo* pServer, const string& authKey)
{
    if (!pServer || !pServer->pClient)
        return;

    TCPPacketWriter writer(TCP_PACKET_HELLO);
    writer.WriteString(authKey);

    pServer->pClient->Send(writer);
}

void ServerManager::SendPlayerPresenceSnapshot(ServerInfo* pServer,
                                               const std::vector<PlayerPresencePacketElement>& elements)
{
    SendArray(pServer, TCP_PACKET_PLAYER_SNAPSHOT, elements);
}

void ServerManager::SendPlayerPresenceUpdate(ServerInfo* pServer,
                                             const std::vector<PlayerPresencePacketElement>& elements)
{
    SendArray(pServer, TCP_PACKET_PLAYER_UPDATE, elements);
}

void ServerManager::SendWorldPresenceSnapshot(ServerInfo* pServer,
                                              const std::vector<WorldPresenceSnapshotElement>& elements)
{
    SendArray(pServer, TCP_PACKET_WORLD_SNAPSHOT, elements);
}

void ServerManager::SendWorldPresenceSnapshotToAll(const WorldPresenceSnapshotElement& snapshotInfo)
{
    std::vector<WorldPresenceSnapshotElement> vec = {snapshotInfo};
    SendArrayToAll(TCP_PACKET_WORLD_SNAPSHOT, vec);
}

void ServerManager::SendWorldPresenceUpdateToAll(const std::vector<WorldPresenceUpdateElement>& elements)
{
    SendArrayToAll(TCP_PACKET_WORLD_UPDATE, elements);
}

void ServerManager::SendWorldPresenceRemoveToAll(const std::vector<WorldPresenceRemoveElement>& elements)
{
    SendArrayToAll(TCP_PACKET_WORLD_REMOVE, elements);
}
bool ServerManager::AddServer(ServerInfo* pServer, uint16 serverID, int8 serverType)
{
    if (!pServer || !pServer->pClient)
        return false;

    if (serverType != CONFIG_SERVER_GAME && serverType != CONFIG_SERVER_RENDERER)
    {
        pServer->pClient->status = SOCKET_CLIENT_CLOSE;
        LOGGER_LOG_WARN("Unknown server %d type %d, closing", serverID, serverType);
        return false;
    }

    auto it = m_servers.find(serverID);
    if (serverID == 0 || it != m_servers.end())
    {
        pServer->pClient->status = SOCKET_CLIENT_CLOSE;
        LOGGER_LOG_ERROR("Server %d already exists but we tried to add it again??", serverID);
        return false;
    }

    auto serverNetInfo = GetContext()->GetGameConfig()->servers[serverID];
    if (serverNetInfo.serverType != serverType || serverNetInfo.lanIP.empty() || serverNetInfo.wanIP.empty())
    {
        pServer->pClient->status = SOCKET_CLIENT_CLOSE;
        return false;
    }

    string serverTypeStr = "game";
    if (serverType == CONFIG_SERVER_RENDERER)
        serverTypeStr = "renderer";

    LOGGER_LOG_INFO("Server %d (%s) added to cache %s:%d", serverID, serverTypeStr.c_str(), serverNetInfo.wanIP.c_str(),
                    serverNetInfo.udpPort);
    pServer->serverID = serverID;
    pServer->wanIP = serverNetInfo.wanIP;
    pServer->port = serverNetInfo.udpPort;
    pServer->serverType = (eConfigServerType)serverType;
    pServer->authed = true;
    pServer->lastHeartbeatTime.Reset();

    m_pendingClients.erase(pServer->GetNetID());
    m_servers.insert_or_assign(pServer->serverID, pServer);

    if (pServer->serverType == CONFIG_SERVER_GAME)
    {
        GetGamePresenceManager()->OnGameServerConnected(pServer);
    }
    return true;
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

    if (pServer->serverType == CONFIG_SERVER_GAME)
    {
        GetGamePresenceManager()->OnGameServerDisconnect(serverID);
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

    for (auto& [_, pServer] : m_servers)
    {
        if (!pServer || pServer->serverType != CONFIG_SERVER_GAME)
        {
            continue;
        }

        float score = (float)pServer->playerCount / (float)(pServer->worldCount + 1.0f);
        if (score < bestScore)
        {
            bestScore = score;
            pBestServer = pServer;
        }
    }

    return pBestServer;
}

ServerInfo* ServerManager::GetBestRenderServer()
{
    for (auto& [_, pServer] : m_servers)
    {
        if (!pServer || pServer->serverType != CONFIG_SERVER_RENDERER)
            continue;

        return pServer;
    }

    return nullptr;
}

bool ServerManager::HasAnyGameServer()
{
    for (auto& [_, pServer] : m_servers)
    {
        if (!pServer || pServer->serverType != CONFIG_SERVER_GAME)
        {
            continue;
        }

        return true;
    }

    return false;
}

uint32 ServerManager::GetPlayerCount()
{
    uint32 playerCount = 0;

    for (auto& [_, pServer] : m_servers)
    {
        if (!pServer)
        {
            continue;
        }

        playerCount += pServer->playerCount;
    }

    return playerCount;
}

void ServerManager::UpdateServers()
{
    if (m_lastServerUpdateTime.GetElapsedTime() < 1000)
        return;

    GetGamePresenceManager()->Update();

    if (m_lastHeartBeatTime.GetElapsedTime() >= 5000)
    {
        PlayerManager* pPlayerMgr = GetPlayerManager();

        for (auto it = m_servers.begin(); it != m_servers.end();)
        {
            ServerInfo* pServer = it->second;

            if (!pServer)
            {
                it = m_servers.erase(it);
                continue;
            }

            if (!pServer->pClient)
            {
                SAFE_DELETE(pServer);
                it = m_servers.erase(it);
                continue;
            }

            if (pServer->lastHeartbeatTime.GetElapsedTime() >= 30 * 1000)
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

    if (m_lastPendingUpdateTime.GetElapsedTime() >= 2000)
    {
        for (auto it = m_pendingClients.begin(); it != m_pendingClients.end();)
        {
            ServerInfo* pServer = it->second;

            if (!pServer)
            {
                it = m_pendingClients.erase(it);
                continue;
            }

            if (!pServer->pClient)
            {
                SAFE_DELETE(pServer);
                it = m_pendingClients.erase(it);
                continue;
            }

            if (pServer->lastHeartbeatTime.GetElapsedTime() >= 60 * 1000)
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

ServerManager* GetServerManager()
{
    return ServerManager::GetInstance();
}
