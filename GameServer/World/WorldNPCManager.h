#pragma once

#include "Precompiled.h"
#include "WorldNPC.h"
#include "Utils/Timer.h"

class World;
class GamePlayer;

enum eNpcEvent
{
    NPC_EVENT_FULL_STATE,
    NPC_EVENT_DELETE,
    NPC_EVENT_ADD,
    NPC_EVENT_MOVE,
    NPC_EVENT_SUCKED,
    NPC_EVENT_BURP,
    NPC_EVENT_TELEPORT,
    NPC_EVENT_DIE,
    NPC_EVENT_PUNCH,
    NPC_EVENT_OUCH,
    NPC_EVENT_ATTACK,
    NPC_EVENT_PREPARE_TO_ATACK
};

struct NeutronBeamInfo
{
    Vector2Float startPos;
    Vector2Float endPos;
    uint32 length = 0;
    uint32 lifeTime = 0;
    int32 ownerNetID = -1;
};

class WorldNPCManager {
public:
    WorldNPCManager(World* pWorld);
    ~WorldNPCManager();

public:
    bool SpawnGhost(eNPCType ghostType, int32 x, int32 y, bool forceSpawn);
    bool SpawnGhostTrap(GamePlayer* pPlayer, int32 x, int32 y);
    void OnNeutronBeam(GamePlayer* pPlayer, const Vector2Float& vStartPos, const Vector2Float& vEndPos);

    void Update();

    void RemoveNpc(int32 index);

    WorldNPC* GetNPCByID(int32 index);
    uint32 GetNPCActiveOrNotCount() const { return m_npcs.size(); }

    void SuckGhostToTrap(WorldNPC* pTrap, float trapWidth, float trapHeight);
    bool IsGhostOnBeam(WorldNPC* pGhost);

    World* GetWorld() const { return m_pWorld; }
    Vector2Float& GetLastLassoePos() { return m_lastLassoePos; }

private:
    bool Spawn(eNPCType npcType, const Vector2Float& pos, const Vector2Float& dest, int32 val1, int32 val2, float speed);

private:
    World* m_pWorld;
    std::vector<WorldNPC> m_npcs;

    std::vector<NeutronBeamInfo> m_neutronBeams;
    Vector2Float m_lastLassoePos;

    Timer m_lastUpdateTime;
    uint32 m_lastIndex;
    uint32 m_activeNpcCount;
};