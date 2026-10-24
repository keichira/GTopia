#pragma once

#include "Precompiled.h"
#include "Utils/Timer.h"

class World;
class GamePlayer;
class TileInfo;

enum eSteamEvent
{
    STEAM_EVENT_START_DOWN,
    STEAM_EVENT_FLIP_VALVE_YES,
    STEAM_EVENT_FLIP_VALVE_NO,
    STEAM_EVENT_VENT,
    STEAM_EVENT_DOOR_OPEN,
    STEAM_EVENT_DOOR_CLOSE,
    STEAM_EVENT_SCRAMBLE_RIGHT,
    STEAM_EVENT_SCRAMBLE_DOWN,
    STEAM_EVENT_SCRAMBLE_LEFT,
    STEAM_EVENT_SCRAMBLE_UP,
    STEAM_EVENT_COLLECTED_NOT_RIGHT,
    STEAM_EVENT_COLLECTED_NOT_DOWN,
    STEAM_EVENT_COLLECTED_NOT_LEFT,
    STEAM_EVENT_COLLECTED_NOT_UP,
    STEAM_EVENT_LAUNCHER_CHARGE,
    STEAM_EVENT_LAUNCHED,
    STEAM_EVENT_START_UP,
    STEAM_EVENT_FAIL,
    STEAM_EVENT_START_RIGHT,
    STEAM_EVENT_START_LEFT,
    STEAM_EVENT_SPIRIT_STORAGE_SUCK_DRY,
    STEAM_EVENT_SPIRIT_STORAGE_SUCK,
    STEAM_EVENT_SPIRIT_STORAGE_BOOM,
    STEAM_EVENT_LAMP,
    STEAM_EVENT_SPIKES_ON,
    STEAM_EVENT_SPIKES_OFF
};

class SteamManager
{
public:
    SteamManager(World* pWorld);

    void AddNode(int32 x, int32 y, uint8 direction);
    void KillNode(TileInfo* pTile);

    int32 GetWaitingNodeCountAt(TileInfo* pTile);

    void Update();

private:
    struct SteamNode
    {
        int32 x = 0;
        int32 y = 0;
        uint8 direction = 0;
        uint8 lifeTime = 0;
        uint8 cooldown = 0;

        bool IsAlive() const { return lifeTime > 0; }
        bool IsWaiting() const { return lifeTime > 0 && cooldown > 0; }
    };

    void Tick();
    void ProcessNode(SteamNode& node, TileInfo* pTile);
    void MoveNode(SteamNode& node, uint8 direction, bool canGo[4]);
    void ReleaseCollectedNodes(TileInfo* pTile, bool canGo[4], uint8 excludedDirection);
    int32 ComputeOpenDirections(SteamNode& node, bool canGo[4]);
    bool IsDirectionOpen(TileInfo* pTarget, int32 direction);
    bool HasAliveNodes();

private:
    SteamNode m_nodes[64];
    Timer m_updateTimer;
    World* m_pWorld;
};