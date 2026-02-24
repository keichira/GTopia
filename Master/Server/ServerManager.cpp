#include "ServerManager.h"
#include "Utils/Timer.h"
#include "IO/Log.h"
#include "../Context.h"

#include "../Event/TCP/TCPEventHello.h"
#include "../Event/TCP/TCPEventAuth.h"

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
        LOGGER_LOG_ERROR("IPHONE KUPI");
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

    m_events.Register(
        TCP_PACKET_HELLO,
        Delegate<NetClient*, VariantVector&>::Create<&TCPEventHello::Execute>()
    );

    m_events.Register(
        TCP_PACKET_AUTH,
        Delegate<NetClient*, VariantVector&>::Create<&TCPEventAuth::Execute>()
    );
}

void ServerManager::AddServer(uint16 serverID, NetClient* pClient)
{
    auto it = m_servers.find(serverID);
    if(it != m_servers.end() || serverID == 0) {
        pClient->status = SOCKET_CLIENT_CLOSE;

        LOGGER_LOG_ERROR("Server %d already exists but we tried to add it again??", serverID);
        return;
    }

    auto serverNetInfo = GetContext()->GetGameConfig()->servers[serverID];
    if(serverNetInfo.lanIP.empty() || serverNetInfo.wanIP.empty()) {
        pClient->status = SOCKET_CLIENT_CLOSE;
        return;
    }

    LOGGER_LOG_INFO("Server %d added to cache %s:%d", serverID, serverNetInfo.wanIP.c_str(), serverNetInfo.udpPort);
    ServerInfo* pServer = new ServerInfo();
    pServer->serverID = serverID;
    pServer->socketConnID = pClient->connectionID;
    pServer->wanIP = serverNetInfo.wanIP;
    pServer->port = serverNetInfo.udpPort;

    m_servers.insert_or_assign(pServer->serverID, pServer);
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

ServerManager* GetServerManager() { return ServerManager::GetInstance(); }
