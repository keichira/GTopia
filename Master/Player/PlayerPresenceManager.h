#pragma once
#include "Precompiled.h"
#include "Utils/Timer.h"
#include <unordered_set>

class NetClient;

struct PlayerPresenceData
{
    uint32 userID;
    uint8 online = 0;
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
    void Subscribe(uint16 serverID, const std::vector<uint32>& userIDs);
    void Unsubscribe(uint16 serverID, const std::vector<uint32>& userIDs);

    void OnTCPPacket(NetClient* pClient, uint16 packetType, const std::vector<uint8>& data);
    
    void OnPlayerStatusChanged(uint32 userID, bool loggedOn);
    void OnGameServerDisconnect(uint16 serverID);

    void Update();

private:
    std::unordered_map<uint32, std::unordered_set<uint16>> m_subscriptions;
    std::unordered_set<uint32> m_dirtyUsers;

    Timer m_flushTimer;
};

PlayerPresenceManager* GetPlayerPresenceManager();