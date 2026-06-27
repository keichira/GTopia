#include "PlayerPresenceManager.h"
#include "PlayerManager.h"
#include "../Server/ServerManager.h"
#include "Network/NetClient.h"

PlayerPresenceManager::PlayerPresenceManager()
{
}

PlayerPresenceManager::~PlayerPresenceManager()
{
}

void PlayerPresenceManager::Subscribe(uint16 serverID, const std::vector<uint32>& userIDs)
{
    ServerInfo* pServer = GetServerManager()->GetServerByID(serverID);
    if(!pServer)
        return;

    std::vector<PlayerPresencePacketElement> snapshotResponse;

    for(auto userID : userIDs) 
    {
        m_subscriptions[userID].insert(serverID);

        PlayerSession* pSession = GetPlayerManager()->GetSessionByID(userID);
        
        uint8 isOnline = 0;
        if(pSession) 
        {
            isOnline = 1; 
        }

        PlayerPresencePacketElement element;
        element.userID = userID;
        element.status = isOnline;

        if(pSession)
        {
            memset(element.name, 0, sizeof(element.name));
            strncpy(element.name, pSession->name.c_str(), sizeof(element.name) - 1);
        }

        snapshotResponse.push_back(std::move(element));
    }

    GetServerManager()->SendPlayerPresenceSnapshot(pServer, snapshotResponse);
}

void PlayerPresenceManager::Unsubscribe(uint16 serverID, const std::vector<uint32>& userIDs)
{
    for(auto userID : userIDs) 
    {
        auto it = m_subscriptions.find(userID);
        if(it != m_subscriptions.end()) 
        {
            it->second.erase(serverID);
            if(it->second.empty()) 
            {
                m_subscriptions.erase(it);
            }
        }
    }
}

void PlayerPresenceManager::OnTCPPacket(NetClient* pClient, uint16 packetType, const std::vector<uint8>& data)
{
    if(!pClient || data.empty() || data.size() % sizeof(uint32) != 0)
        return;

    ServerInfo* pServer = (ServerInfo*)pClient->data;
    if(!pServer)
        return;

    uint32 elementCount = (uint32)(data.size() / sizeof(uint32));
    uint32* pUserIDs = (uint32*)(data.data());

    std::vector<uint32> ids(pUserIDs, pUserIDs + elementCount);
    if (packetType == TCP_PACKET_ONLINE_DATA_SUBSCRIBE) 
    {
        Subscribe(pServer->serverID, ids); 
    } 
    else if (packetType == TCP_PACKET_ONLINE_DATA_UNSUBSCRIBE) 
    {
        Unsubscribe(pServer->serverID, ids);
    }
}

void PlayerPresenceManager::OnPlayerStatusChanged(uint32 userID, bool loggedOn)
{
    if(m_subscriptions.find(userID) != m_subscriptions.end()) 
    {
        m_dirtyUsers.insert(userID);
    }
}

void PlayerPresenceManager::OnGameServerDisconnect(uint16 serverID)
{
}

// todo skip update fro server if theres no heartbeat for long time
void PlayerPresenceManager::Update()
{
    if(m_dirtyUsers.empty()) 
        return;

    if(m_flushTimer.GetElapsedTime() < 5000) 
        return;

    std::unordered_map<uint16, std::vector<PlayerPresencePacketElement>> packetBatches;

    for(auto userID : m_dirtyUsers) 
    {
        auto it = m_subscriptions.find(userID);
        if(it == m_subscriptions.end())
            continue;

        PlayerSession* pSession = GetPlayerManager()->GetSessionByID(userID);

        PlayerPresencePacketElement element;
        element.userID = userID;
        element.status = (pSession ? 1 : 0);

        if(pSession)
        {
            memset(element.name, 0, sizeof(element.name));
            strncpy(element.name, pSession->name.c_str(), sizeof(element.name) - 1);
        }

        for(uint16 serverID : it->second)
        {
            packetBatches[serverID].push_back(std::move(element));
        }
    }

    ServerManager* pServerMgr = GetServerManager();

    for(auto& [serverID, dataList] : packetBatches) 
    {
        ServerInfo* pServer = pServerMgr->GetServerByID(serverID);
        if(!pServer)
            continue;

        GetServerManager()->SendPlayerPresenceUpdate(pServer, dataList);
    }

    m_dirtyUsers.clear();
    m_flushTimer.Reset();
}

PlayerPresenceManager* GetPlayerPresenceManager() { return PlayerPresenceManager::GetInstance(); }
