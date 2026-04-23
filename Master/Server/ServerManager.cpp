#include "ServerManager.h"
#include "Utils/Timer.h"
#include "IO/Log.h"
#include "../Context.h"

#include "../Event/TCP/TCPEventHello.h"
#include "../Event/TCP/TCPEventAuth.h"
#include "../Event/TCP/TCPEventPlayerSession.h"
#include "../Event/TCP/TCPEventWorldInit.h"
#include "../Event/TCP/TCPEventRenderWorld.h"
#include "../Event/TCP/TCPEventWorldSendPlayer.h"
#include "../Event/TCP/TCPEventPlayerEndSession.h"
#include "../Event/TCP/TCPEventKillServer.h"

ServerManager::ServerManager()
{
}

ServerManager::~ServerManager()
{
    Kill();
}

void ServerManager::OnClientConnect(NetClient* pClient)
{

}

void ServerManager::OnClientDisconnect(NetClient* pClient)
{
    if(!pClient || !pClient->data) {
        return;
    }

    NetServerInfo* pNetInfo = (NetServerInfo*)pClient->data;
    RemoveServer(pNetInfo->serverID);
}

void ServerManager::UpdateTCPLogic(uint64 maxTimeMS)
{
    Timer startTime;
    TCPPacketEvent event;

    while(m_packetQueue.try_dequeue(event)) {
        if(!event.pClient) {
            continue;
        }

        int8 type = event.data[0].GetINT();

        switch(type) {
            case TCP_PACKET_HELLO: 
            case TCP_PACKET_AUTH: {
                m_events.Dispatch(type, event.pClient, event.data);
                break;
            }

            default: {
                if(!((NetServerInfo*)(event.pClient->data))->authed) {
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
    while (!m_servers.empty()) {
        RemoveServer(m_servers.begin()->first);
    }

    ServerBroadwayBase::Kill();
    m_servers.clear();
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
}

ServerInfo* ServerManager::GetServerByID(uint16 serverID)
{
    auto it = m_servers.find(serverID);
    if(it == m_servers.end()) {
        return nullptr;
    }

    return it->second;
}

bool ServerManager::SendPacketRaw(uint16 serverID, VariantVector& data)
{
    if(!m_pNetSocket) {
        return false;
    }

    ServerInfo* pServer = GetServerByID(serverID);
    if(!pServer) {
        return false;
    }

    NetClient* pNetClient = m_pNetSocket->GetClient(pServer->socketConnID);
    if(!pNetClient) {
        return false;
    }

    return pNetClient->Send(data);
}

void ServerManager::SendWorldPlayerFailPacket(int32 playerNetID, uint32 serverID)
{
    VariantVector data(3);

    data[0] = TCP_PACKET_WORLD_SEND_PLAYER;
    data[1] = TCP_RESULT_FAIL;
    data[2] = playerNetID;

    SendPacketRaw(serverID, data);
}

void ServerManager::SendWorldPlayerSuccessPacket(int32 playerNetID, uint32 serverID, uint32 worldID, const string& serverIP, uint16 serverPort, uint32 serverIDForPacket)
{
    VariantVector data(7);

    data[0] = TCP_PACKET_WORLD_SEND_PLAYER;
    data[1] = TCP_RESULT_OK;   
    data[2] = playerNetID;
    data[3] = serverID;
    data[4] = worldID;
    data[5] = serverIP;
    data[6] = serverPort;

    SendPacketRaw(serverIDForPacket, data);
}

void ServerManager::SendWorldInitPacket(const string& worldName, uint32 serverID)
{
    VariantVector data(2);

    data[0] = TCP_PACKET_WORLD_INIT;
    data[1] = worldName;

    SendPacketRaw(serverID, data);
}

void ServerManager::SendAuthPacket(bool succeed, uint32 serverID)
{
    VariantVector data(2);

    data[0] = TCP_PACKET_AUTH;
    data[1] = succeed;

    SendPacketRaw(serverID, data);
}

void ServerManager::SendRenderResult(int32 result, uint32 playerUserID, const string& worldName, uint32 serverID)
{
    VariantVector data(5);

    data[0] = TCP_PACKET_RENDER_WORLD;
    data[1] = TCP_RENDER_RESULT;
    data[2] = result;
    data[3] = playerUserID;
    data[4] = worldName;

    SendPacketRaw(serverID, data);
}

void ServerManager::SendRenderRequest(uint32 playerUserID, uint32 worldID, uint32 serverID)
{
    VariantVector data(4);

    data[0] = TCP_PACKET_RENDER_WORLD;
    data[1] = TCP_RENDER_REQUEST;
    data[2] = playerUserID;
    data[3] = worldID;

    SendPacketRaw(serverID, data);
}

void ServerManager::SendPlayerSessionCheck(bool hasSession, int32 playerNetID, int16 connectionID)
{
    NetClient* pClient = m_pNetSocket->GetClient(connectionID);
    if(!pClient) {
        return;
    }

    VariantVector data(3);
    
    data[0] = TCP_PACKET_PLAYER_CHECK_SESSION;
    data[1] = playerNetID;
    data[2] = hasSession;

    pClient->Send(data);
}

void ServerManager::SendHelloPacket(const string &authKey, int16 connectionID)
{
    NetClient* pClient = m_pNetSocket->GetClient(connectionID);
    if(!pClient) {
        return;
    }

    VariantVector data(2);
    
    data[0] = TCP_PACKET_HELLO;
    data[1] = authKey;

    pClient->Send(data);
}

void ServerManager::AddServer(uint16 serverID, NetClient* pClient, int8 serverType)
{
    if(serverType != CONFIG_SERVER_GAME && serverType != CONFIG_SERVER_RENDERER) {
        pClient->status = SOCKET_CLIENT_CLOSE;
        LOGGER_LOG_WARN("Unknown server %d type %d, closing", serverID, serverType);
        return;
    }

    auto it = m_servers.find(serverID);
    if(serverID == 0|| it != m_servers.end()) {
        pClient->status = SOCKET_CLIENT_CLOSE;
        LOGGER_LOG_ERROR("Server %d already exists but we tried to add it again??", serverID);
        return;
    }

    auto serverNetInfo = GetContext()->GetGameConfig()->servers[serverID];
    if(serverNetInfo.serverType != serverType || serverNetInfo.lanIP.empty() || serverNetInfo.wanIP.empty()) {
        pClient->status = SOCKET_CLIENT_CLOSE;
        return;
    }

    string serverTypeStr = "game";
    if(serverType == CONFIG_SERVER_RENDERER) serverTypeStr = "renderer";

    LOGGER_LOG_INFO("Server %d (%s) added to cache %s:%d", serverID, serverTypeStr.c_str(), serverNetInfo.wanIP.c_str(), serverNetInfo.udpPort);
    ServerInfo* pServer = new ServerInfo();
    pServer->serverID = serverID;
    pServer->socketConnID = pClient->connectionID;
    pServer->wanIP = serverNetInfo.wanIP;
    pServer->port = serverNetInfo.udpPort;
    pServer->serverType = (eConfigServerType)serverType;

    m_servers.insert_or_assign(pServer->serverID, pServer);
}

void ServerManager::RemoveServer(uint16 serverID)
{
    auto it = m_servers.find(serverID);
    if(it == m_servers.end()) {
        return;
    }

    ServerInfo* pServer = it->second;

    NetClient* pClient = m_pNetSocket->GetClient(pServer->socketConnID);
    NetServerInfo* pNetInfo = (NetServerInfo*)pClient->data;
    SAFE_DELETE(pNetInfo);

    pClient->status = SOCKET_CLIENT_CLOSE;
    SAFE_DELETE(pServer);
    m_servers.erase(it);
}

ServerInfo* ServerManager::GetBestGameServer()
{
    ServerInfo* pBestServer = nullptr;
    float bestScore = 999999999.0f;

    for(auto& [_, pServer] : m_servers) {
        if(!pServer || pServer->serverType != CONFIG_SERVER_GAME) {
            continue;
        }

        float score = pServer->playerCount / (pServer->worldCount + 1);
        if(score < bestScore) {
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

ServerManager* GetServerManager() { return ServerManager::GetInstance(); }
