#include "GamePresenceManager.h"
#include "../Player/PlayerManager.h"
#include "../Server/ServerManager.h"
#include "Network/NetClient.h"

GamePresenceManager::GamePresenceManager() {}

GamePresenceManager::~GamePresenceManager() {}

void GamePresenceManager::OnTCPPacket(NetClient* pClient, uint16 packetType, const std::vector<uint8>& data)
{
    if (!pClient || data.empty())
        return;

    ServerInfo* pServer = (ServerInfo*)pClient->data;
    if (!pServer)
        return;

    switch (packetType)
    {
        case TCP_PACKET_PLAYER_SUBSCRIBE:
        {
            if (data.size() % sizeof(uint32) != 0)
                return;

            uint32 elementCount = (uint32)(data.size() / sizeof(uint32));
            uint32* pUserIDs = (uint32*)(data.data());
            std::vector<uint32> ids(pUserIDs, pUserIDs + elementCount);
            PlayerSubscribe(pServer->serverID, ids);
            break;
        }

        case TCP_PACKET_PLAYER_UNSUBSCRIBE:
        {
            if (data.size() % sizeof(uint32) != 0)
                return;

            uint32 elementCount = (uint32)(data.size() / sizeof(uint32));
            uint32* pUserIDs = (uint32*)(data.data());
            std::vector<uint32> ids(pUserIDs, pUserIDs + elementCount);
            PlayerUnSubscribe(pServer->serverID, ids);
            break;
        }

        case TCP_PACKET_WORLD_UPDATE:
        {
            if (data.size() % sizeof(WorldPresenceUpdateElement) != 0)
                return;

            uint32 count = (uint32)(data.size() / sizeof(WorldPresenceUpdateElement));
            auto* pElements = (WorldPresenceUpdateElement*)data.data();

            for (uint32 i = 0; i < count; ++i)
            {
                auto it = m_worldSubs.find(pElements[i].worldID);
                if (it != m_worldSubs.end())
                {
                    it->second.info.playerCount = pElements[i].playerCount;
                    it->second.info.isSignalJammed = pElements[i].isSignalJammed;
                    m_dirtyWorlds.insert(pElements[i].worldID);
                }
            }
            break;
        }

        case TCP_PACKET_WORLD_REMOVE:
        {
            if (data.size() % sizeof(WorldPresenceRemoveElement) != 0)
                return;

            auto* pElem = (WorldPresenceRemoveElement*)data.data();
            if (m_worldSubs.erase(pElem->worldID) > 0)
            {
                m_dirtyWorlds.erase(pElem->worldID);
                GetServerManager()->SendRawDataToAllGame(TCP_PACKET_WORLD_REMOVE, (void*)data.data(),
                                                         (uint32)data.size());
            }
            break;
        }
    }
}

void GamePresenceManager::OnPlayerStatusChanged(uint32 userID, bool loggedOn)
{
    if (m_playerSubs.find(userID) != m_playerSubs.end())
    {
        m_dirtyUsers.insert(userID);
    }
}

void GamePresenceManager::OnWorldCreated(uint32 worldID, const string& name, uint16 serverID)
{
    WorldPresenceData& data = m_worldSubs[worldID];
    data.serverID = serverID;
    data.info.worldID = worldID;
    data.info.playerCount = 0;
    data.info.isSignalJammed = 0;

    memset(data.info.name, 0, sizeof(data.info.name));
    strncpy(data.info.name, name.c_str(), sizeof(data.info.name) - 1);

    std::vector<WorldPresenceSnapshotElement> broadcastVec = {data.info};
    GetServerManager()->SendRawDataToAllGame(TCP_PACKET_WORLD_SNAPSHOT, (void*)broadcastVec.data(),
                                             (uint32)(broadcastVec.size() * sizeof(WorldPresenceSnapshotElement)));
}

void GamePresenceManager::OnGameServerDisconnect(uint16 serverID)
{
    std::vector<WorldPresenceRemoveElement> removedWorlds;
    for (auto it = m_worldSubs.begin(); it != m_worldSubs.end();)
    {
        if (it->second.serverID == serverID)
        {
            removedWorlds.push_back(WorldPresenceRemoveElement{it->first});
            m_dirtyWorlds.erase(it->first);
            it = m_worldSubs.erase(it);
        }
        else
        {
            ++it;
        }
    }

    if (!removedWorlds.empty())
    {
        GetServerManager()->SendRawDataToAllGame(TCP_PACKET_WORLD_REMOVE, (void*)removedWorlds.data(),
                                                 (uint32)(removedWorlds.size() * sizeof(WorldPresenceRemoveElement)));
    }
}

void GamePresenceManager::OnGameServerConnected(ServerInfo* pServer)
{
    if (!pServer || !pServer->pClient || m_worldSubs.empty())
        return;

    std::vector<WorldPresenceSnapshotElement> snapshotList;
    snapshotList.reserve(m_worldSubs.size());

    for (auto& [worldID, worldData] : m_worldSubs)
    {
        snapshotList.push_back(worldData.info);
    }

    pServer->pClient->Send(TCP_PACKET_WORLD_SNAPSHOT, snapshotList.data(),
                           (uint32)(snapshotList.size() * sizeof(WorldPresenceSnapshotElement)));
}

void GamePresenceManager::Update()
{
    if (!m_dirtyUsers.empty() && m_playerFlushTimer.GetElapsedTime() >= 5000)
    {
        std::unordered_map<uint16, std::vector<PlayerPresencePacketElement>> packetBatches;

        for (auto userID : m_dirtyUsers)
        {
            auto it = m_playerSubs.find(userID);
            if (it == m_playerSubs.end())
                continue;

            PlayerSession* pSession = GetPlayerManager()->GetSessionByID(userID);

            PlayerPresencePacketElement element;
            element.userID = userID;
            element.status = (pSession ? 1 : 0);

            memset(element.name, 0, sizeof(element.name));
            if (pSession)
            {
                strncpy(element.name, pSession->name.c_str(), sizeof(element.name) - 1);
            }

            for (uint16 serverID : it->second)
            {
                packetBatches[serverID].push_back(element);
            }
        }

        ServerManager* pServerMgr = GetServerManager();

        for (auto& [serverID, dataList] : packetBatches)
        {
            ServerInfo* pServer = pServerMgr->GetServerByID(serverID);
            if (!pServer)
                continue;

            GetServerManager()->SendPlayerPresenceUpdate(pServer, dataList);
        }

        m_dirtyUsers.clear();
        m_playerFlushTimer.Reset();
    }

    if (!m_dirtyWorlds.empty() && m_worldFlushTimer.GetElapsedTime() >= 5000)
    {
        std::vector<WorldPresenceUpdateElement> broadcastVec;
        broadcastVec.reserve(m_dirtyWorlds.size());

        for (uint32 worldID : m_dirtyWorlds)
        {
            auto it = m_worldSubs.find(worldID);
            if (it != m_worldSubs.end())
            {
                broadcastVec.push_back(
                    WorldPresenceUpdateElement{worldID, it->second.info.playerCount, it->second.info.isSignalJammed});
            }
        }

        if (!broadcastVec.empty())
        {
            GetServerManager()->SendRawDataToAllGame(
                TCP_PACKET_WORLD_UPDATE, (void*)broadcastVec.data(),
                (uint32)(broadcastVec.size() * sizeof(WorldPresenceUpdateElement)));
        }

        m_dirtyWorlds.clear();
        m_worldFlushTimer.Reset();
    }
}

void GamePresenceManager::PlayerSubscribe(uint16 serverID, const std::vector<uint32>& userIDs)
{
    ServerInfo* pServer = GetServerManager()->GetServerByID(serverID);
    if (!pServer)
        return;

    std::vector<PlayerPresencePacketElement> snapshotResponse;
    snapshotResponse.reserve(userIDs.size());

    for (auto userID : userIDs)
    {
        m_playerSubs[userID].insert(serverID);

        PlayerSession* pSession = GetPlayerManager()->GetSessionByID(userID);

        PlayerPresencePacketElement element;
        element.userID = userID;
        element.status = (pSession ? 1 : 0);

        memset(element.name, 0, sizeof(element.name));
        if (pSession)
        {
            strncpy(element.name, pSession->name.c_str(), sizeof(element.name) - 1);
        }

        snapshotResponse.push_back(element);
    }

    GetServerManager()->SendPlayerPresenceSnapshot(pServer, snapshotResponse);
}

void GamePresenceManager::PlayerUnSubscribe(uint16 serverID, const std::vector<uint32>& userIDs)
{
    for (auto userID : userIDs)
    {
        auto it = m_playerSubs.find(userID);
        if (it != m_playerSubs.end())
        {
            it->second.erase(serverID);
            if (it->second.empty())
            {
                m_playerSubs.erase(it);
            }
        }
    }
}

GamePresenceManager* GetGamePresenceManager()
{
    return GamePresenceManager::GetInstance();
}