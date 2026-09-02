#pragma once
#include "Packet/GamePacket.h"
#include "Packet/TCPPacket.h"
#include "Precompiled.h"
#include "Utils/Timer.h"
#include <unordered_set>

struct PlayerPresenceData
{
    uint32 userID = 0;
    uint8 status = 0;
    uint16 refCount = 0;
    string name;
};

struct WorldPresenceData
{
    uint32 worldID = 0;
    string name;
    uint16 playerCount = 0;
    bool isSignalJammed = false;
};

class GamePresenceManager
{
public:
    GamePresenceManager();
    ~GamePresenceManager();

    static GamePresenceManager* GetInstance()
    {
        static GamePresenceManager instance;
        return &instance;
    }

public:
    void RequestPresence(const std::vector<uint32>& userIDs, bool isAsap);
    void RequestPresenceForWorld(uint32 worldID, const std::vector<uint32>& userIDs);
    void ReleasePresence(const std::vector<uint32>& userIDs);

    void OnTCPPacket(uint16 packetType, TCPPacketReader& reader);
    void HandleSnapshot(PlayerPresencePacketElement* pElements, uint32 elementCount);

    void Update();

    bool IsSubscribedTo(uint32 userID) { return (m_presences.find(userID) != m_presences.end()); }
    PlayerPresenceData* GetPlayerPresenceData(int32 userID);
    WorldPresenceData* GetWorldPresenceData(uint32 worldID);

    void UpdateLocalWorldPresence(uint32 worldID, uint16 playerCount, bool isSignalJammed);
    void RemoveLocalWorld(uint32 worldID);

    void SortWorldsByPlayerCount();
    uint32 GetWorldPresenceCount() { return m_sortedWorldPresences.size(); }
    WorldPresenceData* GetWorldPresenceDataByIndex(int32 index);
    WorldPresenceData* GetRandomWorldPresenceData(int32 startIndex = 0);

private:
    std::unordered_map<uint32, PlayerPresenceData> m_presences;

    std::unordered_map<uint32, WorldPresenceData> m_worldPresences;
    std::vector<WorldPresenceData> m_sortedWorldPresences;

    std::unordered_map<uint32, std::unordered_set<uint32>> m_pendingPresenceWorlds;
    std::unordered_map<uint32, WorldPresenceUpdateElement> m_localWorldUpdates;
    std::unordered_set<uint32> m_dirtyLocalWorlds;

    std::unordered_set<uint32> m_pendingSubscribeBatch;
    std::unordered_set<uint32> m_pendingUnsubscribeBatch;
    Timer m_subscribeTimer;
    Timer m_unsubscribeTimer;
    Timer m_localWorldFlushTimer;
};

GamePresenceManager* GetGamePresenceManager();