#pragma once

#include "Precompiled.h"
#include "Database/QueryUtils.h"

class ServerInfo;

enum WorldState
{
    WORLD_STATE_LOADING = 0,
    WORLD_STATE_READY   = 1,
    WORLD_STATE_DELETE  = 2
};

struct PendingTransfer
{
    uint32 userID = 0;
    uint32 sourceServerID = 0;
    uint32 targetServerID = 0;
    uint32 worldDatabaseID = 0;
    uint32 worldInstanceID = 0;
    string worldName;
};

struct WorldSession
{
    uint32 instanceID = 0;
    uint32 databaseID = 0;
    uint32 serverID = 0;
    WorldState state = WORLD_STATE_LOADING;
    string worldName;
    std::vector<PendingTransfer> waitingPlayers;
};

class WorldManager {
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
    void HandlePlayerJoinRequest(ServerInfo* pServer, VariantVector&& result);
    void HandleWorldInit(VariantVector&& result);
    static void CheckWorldExistCB(QueryTaskResult&& result);
    static void CreateWorldCB(QueryTaskResult&& result);

    WorldSession* GetWorldByName(const string& worldName);
    WorldSession* GetWorldByInstanceID(uint32 instanceID);

    void EndSessionsByServerID(uint32 serverID);

private:
    void RoutePlayerToExistingWorld(ServerInfo* pSourceServer, uint32 userID, WorldSession& world);
    void CreateWorldSessionAndNotice(uint32 instanceID, uint32 databaseID, const string& worldName, uint32 playerUserID, uint32 sourceServerID);
    void AttachPending(WorldSession* pWorld, uint32 sourceServerID, uint32 userID);
    void FailPlayerJoin(ServerInfo* pServer, uint32 userID, const string& message);

private:
    std::unordered_map<uint32, WorldSession> m_worldSessionsByInstance;
    std::unordered_map<string, uint32> m_worldInstanceByName;
};

WorldManager* GetWorldManager();