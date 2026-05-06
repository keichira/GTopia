#pragma once

#include "Precompiled.h"
#include "../Player/GamePlayer.h"
#include "World.h"
#include "Packet/NetPacket.h"
#include "Event/EventDispatcher.h"
#include <queue>

struct PendingWorldData
{
    World* world;
    uint64 requestTime;
};

struct PendingJoinData
{
    GamePlayer* player;
    uint32 worldInstanceID = 0;
    uint64 requestTime;
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
    void Kill();

    void HandleWorldInit(VariantVector&& result);
    void HandlePlayerJoin(VariantVector&& result);
    void PlayerJoinRequest(GamePlayer* pPlayer, const string& worldName);

    void UpdateWorlds();
    void UpdatePendingLoadWorlds();

    World* GetWorldByInstanceID(uint32 worldID);
    World* GetWorldByName(const string& worldName);
    void AddWorld(World* pWorld);

    void OnHandleGamePacket(ENetEvent& event);
    void SaveAllToDatabase();

    uint32 GetWorldCount() { return m_worlds.size(); }

private:
    template<class T>
    void RegisterPacketEvent(eGamePacketType type)
    {
        m_packetEvents.Register(
            type,
            Delegate<GamePlayer*, World*, GameUpdatePacket*>::Create<&T::Execute>()
        );
    }

    void RegisterEvents();
    void StartWorldLoad(World* pWorld);
    void OnPlayerJoinRequest(GamePlayer* pPlayer, World* pWorld);
    void QueuePlayerToWorld(GamePlayer* pPlayer, World* pWorld);
    void OnWorldReady(World* pWorld);
    void FlushWorldJoinQueue(World* pWorld);

private:
    Timer m_lastWorldUpdateTime;
    std::unordered_map<uint32, World*> m_worlds;
    std::unordered_map<string, uint32> m_worldNameCache;
    std::queue<World*> m_pendingLoad;
    EventDispatcher<eGamePacketType, GamePlayer*, World*, GameUpdatePacket*> m_packetEvents;
};

WorldManager* GetWorldManager();