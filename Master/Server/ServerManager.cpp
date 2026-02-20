#include "ServerManager.h"
#include "Utils/Timer.h"
#include "IO/Log.h"
#include "../Context.h"
#include "../Event/TCP/TCPEventHello.h"

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

void ServerManager::AddServer(uint16 serverID, NetClient* pClient)
{
    auto it = m_servers.find(serverID);
    if(it != m_servers.end()) {
        pClient->status = SOCKET_CLIENT_CLOSE;

        LOGGER_LOG_ERROR("Server %d already exists but we tried to add it again??", serverID);
        return;
    }

    auto serverNetInfo = GetContext()->GetGameConfig()->servers[serverID];
    if(serverNetInfo.lanIP.empty() || serverNetInfo.wanIP.empty()) {
        pClient->status = SOCKET_CLIENT_CLOSE;
        return;
    }

    ServerInfo* pServer = new ServerInfo();
    pServer->serverID = serverID;
    pServer->socketConnID = pClient->connectionID;

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

ServerManager* GetServerManager() { return ServerManager::GetInstance(); }
