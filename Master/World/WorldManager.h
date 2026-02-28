#pragma once

#include "Precompiled.h"
#include "World/WorldManagerBase.h"

enum eWorldState
{
    WORLD_STATE_IDLE,
    WORLD_STATE_LOADING,
    WORLD_STATE_ON
};

struct WorldPendingPlayer
{
    uint16 serverID;
    int32 playerNetID;
};

struct WorldSession
{
    eWorldState state = WORLD_STATE_IDLE;
    
    string worldName = "";
    uint32 worldID = 0;
    uint16 serverID = 0;

    std::vector<WorldPendingPlayer> pendingPlayers;
};

enum eWorldDBState
{
    WORLD_DB_STATE_CHECK,
    WORLD_DB_STATE_CREATE
};

class WorldManager : public WorldManagerBase {
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
    void OnHandleDatabase(QueryTaskResult&& result) override;

public:
    void HandleWorldInit(bool success, uint32 worldID);
    void ManagePlayerJoin(uint16 serverID, int32 playerNetID, const string& worldName);
    void CheckWorldExists(QueryTaskResult&& result);
    void CreateWorld(QueryTaskResult&& result);

    WorldSession* GetWorldByName(const string& worldName);

private:
    std::unordered_map<uint32, WorldSession*> m_worldSession;
};

WorldManager* GetWorldManager();