#include "PlayerPresenceManager.h"
#include "PlayerManager.h"
#include "../Server/MasterBroadway.h"
#include "../World/WorldManager.h"
#include "Math/Random.h"

PlayerPresenceManager::PlayerPresenceManager()
{
    m_unsubscribeTimer.Set(RandomRangeInt(120000, 240000));
}

PlayerPresenceManager::~PlayerPresenceManager()
{
}

void PlayerPresenceManager::RequestPresence(const std::vector<uint32>& userIDs, bool isAsap)
{
    std::vector<uint32> networkSubscribeList;

    for(auto userID : userIDs)
    {
        auto& data = m_presences[userID];
        data.userID = userID;
        data.refCount++;

        if(data.refCount == 1)
        {
            m_pendingUnsubscribeBatch.erase(userID);

            if(isAsap)
                networkSubscribeList.push_back(userID);
            else
                m_pendingSubscribeBatch.insert(userID);
        }
    }

    if(!networkSubscribeList.empty()) 
    {
        GetMasterBroadway()->SendPlayerPresenceSubscribe(networkSubscribeList);
    }
}

void PlayerPresenceManager::RequestPresenceForWorld(uint32 worldID, const std::vector<uint32>& userIDs)
{
    auto& list = m_pendingPresenceWorlds[worldID];
    for(auto userID : userIDs)
        list.insert(userID);

    RequestPresence(userIDs, true);
}

void PlayerPresenceManager::ReleasePresence(const std::vector<uint32>& userIDs)
{
    for(auto userID : userIDs) 
    {
        auto it = m_presences.find(userID);
        if(it != m_presences.end()) 
        {
            it->second.refCount--;
            
            if(it->second.refCount == 0) 
            {
                m_pendingSubscribeBatch.erase(userID);
                m_pendingUnsubscribeBatch.insert(userID);
            }
        }
    }
}

void PlayerPresenceManager::OnTCPPacket(uint16 packetType, const std::vector<uint8>& data)
{    
    if(data.empty() || data.size() % sizeof(PlayerPresencePacketElement) != 0)
        return;

    uint32 elementCount = (uint32)(data.size() / sizeof(PlayerPresencePacketElement));
    PlayerPresencePacketElement* pElements = (PlayerPresencePacketElement*)(data.data());

    for(uint32 i = 0; i < elementCount; ++i)
    {
        uint32 uID = pElements[i].userID;
        uint8 status = pElements[i].status;

        auto it = m_presences.find(uID);
        if(it != m_presences.end()) 
        {
            it->second.status = status;
            it->second.name = pElements[i].name;
        }
    }

    if(packetType == TCP_PACKET_ONLINE_DATA_SNAPSHOT) 
    {
        HandleSnapshot(pElements, elementCount);
    }
    else
    {
        // todo we can trigger some funcs in here like friend data
    }
}

void PlayerPresenceManager::HandleSnapshot(PlayerPresencePacketElement* pElements, uint32 elementCount)
{
    if(!pElements || elementCount == 0)
        return;

    for(auto it = m_pendingPresenceWorlds.begin(); it != m_pendingPresenceWorlds.end(); )
    {
        uint32 instanceID = it->first;
        auto& list = it->second;

        for(uint32 i = 0; i < elementCount; ++i)
        {
            list.erase(pElements[i].userID);
        }

        if(list.empty())
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

PlayerPresenceData* PlayerPresenceManager::GetPlayerPresenceData(int32 userID)
{
    auto it = m_presences.find(userID);
    if(it == m_presences.end())
        return nullptr;

    return &it->second;
}

void PlayerPresenceManager::Update()
{
    if(m_subscribeTimer.GetElapsedTime() >= 2000) 
    {
        if(!m_pendingSubscribeBatch.empty())
        {
            std::vector<uint32> finalSubList;
            for(uint32 userID : m_pendingSubscribeBatch)
            {
                auto it = m_presences.find(userID);
                if(it != m_presences.end() && it->second.refCount > 0)
                    finalSubList.push_back(userID);
            }
            
            if(!finalSubList.empty())
            {
                GetMasterBroadway()->SendPlayerPresenceSubscribe(finalSubList);
            }
                
            m_pendingSubscribeBatch.clear();
        }

        m_subscribeTimer.Reset();
    }

    if(m_unsubscribeTimer.IsPassed())
    {
        if(!m_pendingUnsubscribeBatch.empty())
        {
            std::vector<uint32> finalUnsubList;
            
            for(uint32 userID : m_pendingUnsubscribeBatch)
            {
                auto it = m_presences.find(userID);
                if(it != m_presences.end() && it->second.refCount == 0)
                {
                    finalUnsubList.push_back(userID);
                    m_presences.erase(it);
                }
            }

            if(!finalUnsubList.empty())
            {
                GetMasterBroadway()->SendPlayerPresenceUnsubscribe(finalUnsubList);
            }

            m_pendingUnsubscribeBatch.clear();
        }

        m_unsubscribeTimer.Set(RandomRangeInt(120000, 240000));
    }
}

PlayerPresenceManager* GetPlayerPresenceManager() { return PlayerPresenceManager::GetInstance(); }