#pragma once
#include "Packet/GamePacket.h"
#include "Precompiled.h"
#include "Utils/Timer.h"
#include <unordered_set>

class NetClient;
class ServerInfo;

struct PlayerPresenceData
{
    uint32 userID;
    uint8 online = 0;
};

struct WorldPresenceData
{
    WorldPresenceSnapshotElement info;
    uint16 serverID = 0;
};

class GamePresenceManager
{
public:
    GamePresenceManager();
    ~GamePresenceManager();

public:
    static GamePresenceManager* GetInstance()
    {
        static GamePresenceManager instance;
        return &instance;
    }

public:
    void OnTCPPacket(NetClient* pClient, uint16 packetType, const std::vector<uint8>& data);

    void OnPlayerStatusChanged(uint32 userID, bool loggedOn);
    void OnWorldCreated(uint32 worldID, const string& name, uint16 serverID);

    void OnGameServerDisconnect(uint16 serverID);
    void OnGameServerConnected(ServerInfo* pServer);

    void Update();

private:
    void PlayerSubscribe(uint16 serverID, const std::vector<uint32>& userIDs);
    void PlayerUnSubscribe(uint16 serverID, const std::vector<uint32>& userIDs);

private:
    std::unordered_map<uint32, std::unordered_set<uint16>> m_playerSubs;
    std::unordered_set<uint32> m_dirtyUsers;
    Timer m_playerFlushTimer;

    std::unordered_map<uint32, WorldPresenceData> m_worldSubs;
    std::unordered_set<uint32> m_dirtyWorlds;
    Timer m_worldFlushTimer;
};

GamePresenceManager* GetGamePresenceManager();