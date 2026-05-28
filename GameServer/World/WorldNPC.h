#pragma once

#include "Precompiled.h"
#include "Math/Vector2.h"

class WorldNPCManager;

enum eNPCType
{
    NPC_TYPE_NONE,
    NPC_TYPE_GHOST,
    NPC_TYPE_GHOST_JAR,
    NPC_TYPE_BEE_SWARM,
    NPC_TYPE_HARVEST_GHOST,
    NPC_TYPE_GROWGA,
    NPC_TYPE_GHOST_SHARK,
    NPC_TYPE_XMAS_GHOST,
    NPC_TYPE_BLAST,
    NPC_TYPE_PINATA,
    NPC_TYPE_GHOST_CAPTURE_MACHINE,
    NPC_TYPE_BOSS_GHOST,
    NPC_TYPE_MIND_CONTROL_GHOST,
    NPC_TYPE_GHOST_BE_GONE,
    NPC_TYPE_HUNTED_TURKEY,
    NPC_TYPE_TRICKSTER
};

class WorldNPC {
public:
    WorldNPC();
    ~WorldNPC();

public:
    void Init(eNPCType npcType, uint8 npcID, const Vector2Float& npcPos);
    void Update(WorldNPCManager* pNPCMgr, uint64 deltaMS);

    void MoveGhostRandom(WorldNPCManager* pNPCMgr);
    void CheckGhostCanSlime(WorldNPCManager* pNPCMgr);

    bool IsGhost();
    bool IsGhostTrap();
    bool IsInside(WorldNPC* pNpc, float padX, float padY);

public:
    uint8 id;
    uint8 type;
    Vector2Float pos;
    Vector2Float dest;
    float speed;
    int32 val1;
    int32 val2;

    bool lassoed;
};