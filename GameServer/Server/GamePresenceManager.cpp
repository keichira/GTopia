#include "GamePresenceManager.h"
#include "../Player/PlayerManager.h"
#include "../Server/MasterBroadway.h"
#include "../World/WorldManager.h"
#include "Math/Random.h"

GamePresenceManager::GamePresenceManager()
{
    m_unsubscribeTimer.Set(RandomRangeInt(120000, 240000));
}

GamePresenceManager::~GamePresenceManager() {}

void GamePresenceManager::RequestPresence(const std::vector<uint32>& userIDs, bool isAsap)
{
    std::vector<uint32> networkSubscribeList;

    for (auto userID : userIDs)
    {
        auto& data = m_presences[userID];
        data.userID = userID;
        data.refCount++;

        if (data.refCount == 1)
        {
            m_pendingUnsubscribeBatch.erase(userID);

            if (isAsap)
                networkSubscribeList.push_back(userID);
            else
                m_pendingSubscribeBatch.insert(userID);
        }
    }

    if (!networkSubscribeList.empty())
    {
        GetMasterBroadway()->SendPlayerPresenceSubscribe(networkSubscribeList);
    }
}

void GamePresenceManager::RequestPresenceForWorld(uint32 worldID, const std::vector<uint32>& userIDs)
{
    auto& list = m_pendingPresenceWorlds[worldID];
    for (auto userID : userIDs)
        list.insert(userID);

    RequestPresence(userIDs, true);
}

void GamePresenceManager::ReleasePresence(const std::vector<uint32>& userIDs)
{
    for (auto userID : userIDs)
    {
        auto it = m_presences.find(userID);
        if (it != m_presences.end())
        {
            if (it->second.refCount > 0)
                it->second.refCount--;

            if (it->second.refCount == 0)
            {
                m_pendingSubscribeBatch.erase(userID);
                m_pendingUnsubscribeBatch.insert(userID);
            }
        }
    }
}

void GamePresenceManager::OnTCPPacket(uint16 packetType, TCPPacketReader& reader)
{
    switch (packetType)
    {
        case TCP_PACKET_PLAYER_SNAPSHOT:
        case TCP_PACKET_PLAYER_UPDATE:
        {
            std::vector<PlayerPresencePacketElement> elements;
            if (!reader.ReadArray(elements) || elements.empty())
                return;

            for (auto& elem : elements)
            {
                auto it = m_presences.find(elem.userID);
                if (it != m_presences.end())
                {
                    it->second.status = elem.status;
                    it->second.name = elem.name;
                }
            }

            if (packetType == TCP_PACKET_PLAYER_SNAPSHOT)
            {
                HandleSnapshot(elements.data(), (uint32)elements.size());
            }
            break;
        }

        case TCP_PACKET_WORLD_SNAPSHOT:
        {
            std::vector<WorldPresenceSnapshotElement> elements;
            if (!reader.ReadArray(elements) || elements.empty())
                return;

            for (auto& elem : elements)
            {
                auto& wData = m_worldPresences[elem.worldID];
                wData.worldID = elem.worldID;
                wData.name = elem.name;
                wData.playerCount = elem.playerCount;
                wData.isSignalJammed = (elem.isSignalJammed != 0);
            }
            break;
        }

        case TCP_PACKET_WORLD_UPDATE:
        {
            std::vector<WorldPresenceUpdateElement> elements;
            if (!reader.ReadArray(elements) || elements.empty())
                return;

            for (auto& elem : elements)
            {
                auto it = m_worldPresences.find(elem.worldID);
                if (it != m_worldPresences.end())
                {
                    it->second.playerCount = elem.playerCount;
                    it->second.isSignalJammed = (elem.isSignalJammed != 0);
                }
            }
            break;
        }

        case TCP_PACKET_WORLD_REMOVE:
        {
            uint32 worldID = 0;
            if (reader.Read<uint32>(worldID))
            {
                m_worldPresences.erase(worldID);
            }
            break;
        }
    }
}

void GamePresenceManager::HandleSnapshot(PlayerPresencePacketElement* pElements, uint32 elementCount)
{
    if (!pElements || elementCount == 0)
        return;

    for (auto it = m_pendingPresenceWorlds.begin(); it != m_pendingPresenceWorlds.end();)
    {
        uint32 instanceID = it->first;
        auto& list = it->second;

        for (uint32 i = 0; i < elementCount; ++i)
        {
            list.erase(pElements[i].userID);
        }

        if (list.empty())
        {
            GetWorldManager()->OnWorldPresenceReady(instanceID);
            it = m_pendingPresenceWorlds.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

PlayerPresenceData* GamePresenceManager::GetPlayerPresenceData(int32 userID)
{
    auto it = m_presences.find(userID);
    if (it == m_presences.end())
        return nullptr;

    return &it->second;
}

WorldPresenceData* GamePresenceManager::GetWorldPresenceData(uint32 worldID)
{
    auto it = m_worldPresences.find(worldID);
    if (it == m_worldPresences.end())
        return nullptr;

    return &it->second;
}

void GamePresenceManager::UpdateLocalWorldPresence(uint32 worldID, uint16 playerCount, bool isSignalJammed)
{
    auto& updateData = m_localWorldUpdates[worldID];
    if (updateData.playerCount == playerCount && updateData.isSignalJammed == (isSignalJammed ? 1 : 0))
        return;

    updateData.worldID = worldID;
    updateData.playerCount = playerCount;
    updateData.isSignalJammed = isSignalJammed ? 1 : 0;

    m_dirtyLocalWorlds.insert(worldID);
}

void GamePresenceManager::RemoveLocalWorld(uint32 worldID)
{
    m_localWorldUpdates.erase(worldID);
    m_dirtyLocalWorlds.erase(worldID);

    WorldPresenceRemoveElement removeElem;
    removeElem.worldID = worldID;

    GetMasterBroadway()->SendWorldPresenceRemove({removeElem});
}

void GamePresenceManager::SortWorldsByPlayerCount()
{
    m_sortedWorldPresences.clear();
    m_sortedWorldPresences.reserve(m_worldPresences.size());

    for (auto& pair : m_worldPresences)
    {
        m_sortedWorldPresences.push_back(pair.second);
    }

    std::sort(m_sortedWorldPresences.begin(), m_sortedWorldPresences.end(),
              [](const WorldPresenceData& a, const WorldPresenceData& b) { return a.playerCount > b.playerCount; });
}

WorldPresenceData* GamePresenceManager::GetWorldPresenceDataByIndex(int32 index)
{
    if (index < 0 || index >= (int32)m_sortedWorldPresences.size())
        return nullptr;

    return &m_sortedWorldPresences[index];
}

WorldPresenceData* GamePresenceManager::GetRandomWorldPresenceData(int32 startIndex)
{
    if (startIndex < 0 || startIndex >= (int32)m_sortedWorldPresences.size())
        return nullptr;

    int32 randomIndex = RandomRangeInt(startIndex, m_sortedWorldPresences.size() - 1);
    return &m_sortedWorldPresences[randomIndex];
}

void GamePresenceManager::Update()
{
    if (m_subscribeTimer.GetElapsedTime() >= 5000)
    {
        if (!m_pendingSubscribeBatch.empty())
        {
            std::vector<uint32> finalSubList;
            for (uint32 userID : m_pendingSubscribeBatch)
            {
                auto it = m_presences.find(userID);
                if (it != m_presences.end() && it->second.refCount > 0)
                    finalSubList.push_back(userID);
            }

            if (!finalSubList.empty())
            {
                GetMasterBroadway()->SendPlayerPresenceSubscribe(finalSubList);
            }

            m_pendingSubscribeBatch.clear();
        }

        m_subscribeTimer.Reset();
    }

    if (m_unsubscribeTimer.IsPassed())
    {
        if (!m_pendingUnsubscribeBatch.empty())
        {
            std::vector<uint32> finalUnsubList;

            for (uint32 userID : m_pendingUnsubscribeBatch)
            {
                auto it = m_presences.find(userID);
                if (it != m_presences.end() && it->second.refCount == 0)
                {
                    finalUnsubList.push_back(userID);
                    m_presences.erase(it);
                }
            }

            if (!finalUnsubList.empty())
            {
                GetMasterBroadway()->SendPlayerPresenceUnsubscribe(finalUnsubList);
            }

            m_pendingUnsubscribeBatch.clear();
        }

        m_unsubscribeTimer.Set(RandomRangeInt(120000, 240000));
    }

    if (!m_dirtyLocalWorlds.empty() && m_localWorldFlushTimer.GetElapsedTime() >= 5000)
    {
        std::vector<WorldPresenceUpdateElement> updates;
        updates.reserve(m_dirtyLocalWorlds.size());

        for (uint32 worldID : m_dirtyLocalWorlds)
        {
            updates.push_back(m_localWorldUpdates[worldID]);
        }

        if (!updates.empty())
        {
            GetMasterBroadway()->SendWorldPresenceUpdate(updates);
        }

        m_dirtyLocalWorlds.clear();
        m_localWorldFlushTimer.Reset();
    }
}

GamePresenceManager* GetGamePresenceManager()
{
    return GamePresenceManager::GetInstance();
}