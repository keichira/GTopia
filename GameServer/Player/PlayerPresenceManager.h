#pragma once
#include "Precompiled.h"
#include "Packet/GamePacket.h"
#include <unordered_set>
#include "Utils/Timer.h"

struct PlayerPresenceData
{
    uint32 userID;
    uint8 status;
    uint16 refCount = 0;
    string name;
};

class PlayerPresenceManager {
public:
    PlayerPresenceManager();
    ~PlayerPresenceManager();

public:
    static PlayerPresenceManager* GetInstance()
    {
        static PlayerPresenceManager instance;
        return &instance;
    }

public:
    void RequestPresence(const std::vector<uint32>& userIDs, bool isAsap);
    void RequestPresenceForWorld(uint32 worldID, const std::vector<uint32>& userIDs);
    void ReleasePresence(const std::vector<uint32>& userIDs);

    void OnTCPPacket(uint16 packetType, const std::vector<uint8>& data);
    
    void HandleSnapshot(PlayerPresencePacketElement* pElements, uint32 elementCount);
    //void HandleUpdate(const std::vector<PlayerPresencePacketElement>& data);

    void Update();

    bool IsSubscribedTo(uint32 userID)
    {
        return (m_presences.find(userID) != m_presences.end());
    }

    PlayerPresenceData* GetPlayerPresenceData(int32 userID);

private:
    std::unordered_map<uint32, PlayerPresenceData> m_presences;
    std::unordered_map<uint32, std::unordered_set<uint32>> m_pendingPresenceWorlds;

    std::unordered_set<uint32> m_pendingSubscribeBatch;
    std::unordered_set<uint32> m_pendingUnsubscribeBatch;
    Timer m_subscribeTimer;
    Timer m_unsubscribeTimer;
};

PlayerPresenceManager* GetPlayerPresenceManager();