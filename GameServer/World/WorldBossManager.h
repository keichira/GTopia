#pragma once

#include "Precompiled.h"

class World;

enum eWorldBossType
{
    WORLD_BOSS_TYPE_NONE,
    WORLD_BOSS_TYPE_GHOST
};

struct WorldBossInfo 
{
    string startNotifyMsg;
    string startImage;
    string startAudio;

    string endNotifyMsg;
    string endImage;
    string endAudio;

    uint32 hp = 0;
};

struct WorldBoss 
{
    eWorldBossType bossType = WORLD_BOSS_TYPE_NONE;
    uint8 npcID = 0;
    std::vector<uint8> npcs;
};

class WorldBossManager {
public:
    WorldBossManager(World* pWorld);
    ~WorldBossManager();

public:
    bool StartBoss(eWorldBossType type);
    void EndBoss(uint8 npcID);

    WorldBoss* GetBossByNpcID(uint8 npcID);
    
    void Update();

private:
    std::vector<WorldBossInfo> m_bossInfo;
    std::vector<WorldBoss> m_bosses;
    World* m_pWorld;
};