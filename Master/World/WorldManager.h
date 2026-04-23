#pragma once

#include "Precompiled.h"
#include "Database/QueryUtils.h"

enum eWorldState
{
    WORLD_STATE_IDLE,
    WORLD_STATE_LOADING,
    WORLD_STATE_ON
};

struct WorldPendingPlayer
{
    uint32 serverID = 0;
    int32 playerNetID = 0;
};

struct WorldSession
{
    eWorldState state = WORLD_STATE_IDLE;
    
    string worldName = "";
    uint32 worldID = 0;
    uint32 serverID = 0;

    std::vector<WorldPendingPlayer> pendingPlayers;

    void AddPending(uint32 serverID, int32 netID)
    {
        pendingPlayers.emplace_back(WorldPendingPlayer{serverID, netID});
    }
};

class WorldManager {
private:
    enum eWorldDatabaseState
    {
        WORLD_DB_STATE_CHECK_EXISTS,
        WORLD_DB_STATE_CREATE
    };

public:
    WorldManager();
    ~WorldManager();

public:
    static WorldManager* GetInstance()
    {
        static WorldManager instance;
        return &instance;
    }

public:
    void HandleWorldInit(VariantVector&& result);

    static void CheckWorldExistCB(QueryTaskResult&& result);
    static void CreateWorldCB(QueryTaskResult&& result);

    void HandlePlayerJoinRequest(VariantVector&& result);

    void CreateWorldSessionAndNotice(uint32 worldID, const string& worldName, int32 playerNetID, uint32 serverID);

    WorldSession* GetWorldByName(const string& worldName);
    WorldSession* GetWorldByID(uint32 worldID);

    void RemoveWorldsWithServerID(uint32 serverID);

private:
    std::unordered_map<uint32, WorldSession> m_worldSessions;
};

WorldManager* GetWorldManager();