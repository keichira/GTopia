#include "ServerManager.h"
#include "Utils/Timer.h"
#include "IO/Log.h"
#include "../Context.h"

#include "../Event/TCP/TCPEventHello.h"
#include "../Event/TCP/TCPEventAuth.h"
#include "../Event/TCP/TCPEventPlayerSession.h"
#include "../Event/TCP/TCPEventWorldInit.h"
#include "../Event/TCP/TCPEventRenderWorld.h"
#include "../Event/TCP/TCPEventRenderWorldRes.h"

ServerManager::ServerManager()
{
}

ServerManager::~ServerManager()
{
    Kill();
}

void ServerManager::OnClientConnect(NetClient* pClient)
{
    if(!pClient) {
        return;
    }
}

void ServerManager::OnClientDisconnect(NetClient* pClient)
{

}

void ServerManager::UpdateTCPLogic(uint64 maxTimeMS)
{
    uint64 startTime = Time::GetSystemTime();
    TCPPacketEvent event;

    uint32 processed = 0;

    while(m_packetQueue.try_dequeue(event)) {
        LOGGER_LOG_DEBUG("IPHONE KUPI");
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
                if(!((NetClientInfo*)(event.pClient->data))->authed) {
                    LOGGER_LOG_WARN("Client trying to access un-authorized packets?! CLOSING!");
                    event.pClient->status = SOCKET_CLIENT_CLOSE;
                    continue;
                }

                m_events.Dispatch(type, event.pClient, event.data);
            }
        }

        processed++;
        if(Time::GetSystemTime() - startTime >= maxTimeMS) {
            break;
        }
    }

    if(processed > 0) {
        LOGGER_LOG_DEBUG("Processed %d TCP packets maxMS %d, took %d MS", processed, maxTimeMS, Time::GetSystemTime() - startTime);
    }
}

void ServerManager::Kill()
{
    ServerBroadwayBase::Kill();

    for(auto& [_, pServer] : m_servers) {
        SAFE_DELETE(pServer);
    }

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
    RegisterEvent<TCPEventRenderWorldRes>(TCP_PACKET_RENDER_WORLD_RES);
}

ServerInfo* ServerManager::GetServerByID(uint16 serverID)
{
    auto it = m_servers.find(serverID);
    if(it == m_servers.end()) {
        return nullptr;
    }

    return it->second;
}

RendererInfo* ServerManager::GetRendererByID(uint rendererID)
{
    for(auto& pRenderer : m_renderers) {
        if(pRenderer->serverID == rendererID) {
            return pRenderer;
        }
    }

    return nullptr;
}

bool ServerManager::SendPacketServerRaw(uint16 serverID, VariantVector& data)
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

bool ServerManager::SendPacketRendererRaw(uint16 rendererID, VariantVector &data)
{
    if(!m_pNetSocket) {
        return false;
    }

    RendererInfo* pServer = GetRendererByID(rendererID);
    if(!pServer) {
        return false;
    }

    NetClient* pNetClient = m_pNetSocket->GetClient(pServer->socketConnID);
    if(!pNetClient) {
        return false;
    }

    return pNetClient->Send(data);
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
    if(serverType == CONFIG_SERVER_GAME) {
        ServerInfo* pServer = new ServerInfo();
        pServer->serverID = serverID;
        pServer->socketConnID = pClient->connectionID;
        pServer->wanIP = serverNetInfo.wanIP;
        pServer->port = serverNetInfo.udpPort;
    
        m_servers.insert_or_assign(pServer->serverID, pServer);
    }
    else if(serverType == CONFIG_SERVER_RENDERER) {
        RendererInfo* pServer = new RendererInfo();
        pServer->serverID = serverID;
        pServer->socketConnID = pClient->connectionID;
        pServer->wanIP = serverNetInfo.wanIP;
        pServer->port = serverNetInfo.udpPort;
    
        m_renderers.push_back(pServer);
    }
}

void ServerManager::RemoveServer(uint16 serverID)
{
    auto it = m_servers.find(serverID);
    if(it == m_servers.end()) {
        return;
    }

    SAFE_DELETE(it->second);
    m_servers.erase(it);
}

ServerInfo* ServerManager::GetBestServer()
{
    if(m_servers.empty()) {
        return nullptr;
    }

    return m_servers[1];
}

RendererInfo* ServerManager::GetBestRenderer()
{
    if(m_renderers.empty()) {
        return nullptr;
    }

    return m_renderers[0];
}

ServerManager* GetServerManager() { return ServerManager::GetInstance(); }
