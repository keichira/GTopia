#pragma once

#include "../Player/GamePlayer.h"
#include "Event/EventDispatcher.h"
#include "Packet/NetPacket.h"
#include "Packet/TCPPacket.h"
#include "Precompiled.h"
#include "World.h"
#include "World/WorldBalancer.h"
#include <queue>
#include <unordered_set>

class WorldManager : public WorldBalancer
{
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

    void HandleWorldInit(TCPPacketReader& reader);
    void HandlePlayerJoin(TCPPacketReader& reader);
    void PlayerJoinRequest(GamePlayer* pPlayer, const string& worldName);

    void UpdateWorlds();
    void UpdatePendingLoadWorlds();
    void OnWorldPresenceReady(uint32 worldID);

    World* GetWorldByInstanceID(uint32 worldID);
    World* GetWorldByName(const string& worldName);
    World* GetWorldByDatabaseID(uint32 databaseID);
    void AddWorld(World* pWorld);

    void OnHandleGamePacket(NetworkEvent& event);
    void SaveAllToDatabase();

    void SendWorldMenuRequest(GamePlayer* pPlayer);
    uint32 GetWorldCount() { return m_worlds.size(); }

private:
    template <class T> void RegisterPacketEvent(eGamePacketType type)
    {
        m_packetEvents.Register(type, Delegate<GamePlayer*, World*, GameUpdatePacket*>::Create<&T::Execute>());
    }

    void RegisterEvents();
    void StartWorldLoad(World* pWorld);
    void OnPlayerJoinRequest(GamePlayer* pPlayer, World* pWorld);

private:
    Timer m_lastWorldUpdateTime;
    std::unordered_map<uint32, World*> m_worlds;
    std::unordered_map<string, uint32> m_worldNameCache;
    std::queue<World*> m_pendingLoad;
    EventDispatcher<eGamePacketType, GamePlayer*, World*, GameUpdatePacket*> m_packetEvents;

    struct WorldListElement
    {
        string name;
        uint16 playerCount = 0;
    };
    std::vector<WorldListElement> m_cachedPopularWorldList;
    Timer m_worldListCacheTimer;
};

WorldManager* GetWorldManager();