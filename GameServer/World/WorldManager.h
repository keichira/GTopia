#pragma once

#include "Precompiled.h"
#include "../Player/GamePlayer.h"
#include "World.h"
#include "Packet/NetPacket.h"
#include "Event/EventDispatcher.h"

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
    void PlayerJoinRequest(GamePlayer* pPlayer, const string& worldName);
    void PlayerLeaveWorld(GamePlayer* pPlayer);

    void Kill();

    void HandleWorldInit(VariantVector&& result);
    static void WorldDBInitCB(QueryTaskResult&& result);
    void HandlePlayerJoin(VariantVector&& result);

    World* GetWorldByID(uint32 worldID);
    World* GetWorldByName(const string& worldName);
    void AddWorld(World* pWorld);

    void RegisterEvents();
    void OnHandleGamePacket(ENetEvent& event);

    void UpdateWorlds();
    void ForceSaveAllWorlds();

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

private:
    Timer m_lastWorldUpdateTime;

    std::unordered_map<uint32, World*> m_worlds;
    std::vector<World*> m_pendingDelete;
    EventDispatcher<eGamePacketType, GamePlayer*, World*, GameUpdatePacket*> m_packetEvents;
};

WorldManager* GetWorldManager();