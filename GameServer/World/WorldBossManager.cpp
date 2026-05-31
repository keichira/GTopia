#include "WorldBossManager.h"
#include "World.h"

WorldBossManager::WorldBossManager(World* pWorld)
{
    m_pWorld = pWorld;

    WorldBossInfo ghostBoss;
    ghostBoss.startImage = "interface/large/slobby_aprs.rttex";
    ghostBoss.startNotifyMsg = "BOO! A Boss Ghost has appeared in the world!";
    ghostBoss.startAudio = "boo_evil_laugh.wav";

    ghostBoss.endImage = "interface/large/slobby_ded.rttex";
    ghostBoss.endNotifyMsg = "The Boss Ghost has been busted!";
    ghostBoss.endAudio = "boo_evil_laugh.wav";
    ghostBoss.hp = 50000;

    m_bossInfo.push_back(std::move(ghostBoss));
}

WorldBossManager::~WorldBossManager()
{
}

bool WorldBossManager::StartBoss(eWorldBossType type)
{
    if(!m_pWorld || type == WORLD_BOSS_TYPE_NONE)
        return false;

    Vector2Int& vWorldSize = m_pWorld->GetTileManager()->GetSize();
    
    float spawnX = RandomRangeFloat(0, vWorldSize.x);
    float spawnY = RandomRangeFloat(vWorldSize.y / 4, vWorldSize.y / 2);

    eNPCType npcType = NPC_TYPE_NONE;
    
    switch(type)
    {
        case WORLD_BOSS_TYPE_GHOST:
            npcType = NPC_TYPE_BOSS_GHOST;
            break;
    }

    if(!m_pWorld->GetNPCManager()->SpawnGhost(npcType, spawnX, spawnY, false))
        return false;

    WorldBoss boss;
    boss.bossType = type;
    boss.npcID = m_pWorld->GetNPCManager()->GetLastAddedNPCID();

    if((type - 1) < m_bossInfo.size())
    {
        WorldBossInfo& bossInfo = m_bossInfo[type - 1];
        m_pWorld->SendOnAddNotificationToAll(bossInfo.startImage, bossInfo.startNotifyMsg, bossInfo.startAudio, false);

        WorldNPC* pNpc = m_pWorld->GetNPCManager()->GetNPCByID(boss.npcID);
        if(pNpc)
        {
            pNpc->hp = bossInfo.hp;
        }
    }

    m_bosses.push_back(std::move(boss));
    return true;
}

WorldBoss* WorldBossManager::GetBossByNpcID(uint8 npcID)
{
    for(auto& boss : m_bosses)
    {
        if(boss.npcID == npcID)
            return &boss;
    }

    return nullptr;
}

void WorldBossManager::Update()
{
    if(!m_pWorld || m_bosses.empty())
        return;

    for(auto& boss : m_bosses)
    {
        switch(boss.bossType)
        {
            case WORLD_BOSS_TYPE_GHOST:
            {
                WorldNPC* pNpc = m_pWorld->GetNPCManager()->GetNPCByID(boss.npcID);
                if(!pNpc)
                {
                    EndBoss(boss.npcID);
                    continue;
                }

                //m_pWorld->GetTileManager()->RebuildPowerNodeGroups();
                
                if(m_pWorld->GetTileManager()->CheckIfPointInsidePowerNodeGroups(pNpc->pos))
                {
                    pNpc->isInsidePowerNodes = true;
                }
                else
                {
                    pNpc->isInsidePowerNodes = false;
                }
                break;
            }
        }
    }
}

void WorldBossManager::EndBoss(uint8 npcID)
{
    if(!m_pWorld)
        return;

    WorldBoss* pBoss = GetBossByNpcID(npcID);
    if(!pBoss)
        return;

    m_pWorld->GetNPCManager()->RemoveNpc(npcID);

    if(pBoss->bossType == WORLD_BOSS_TYPE_NONE)
        return;

    WorldBossInfo& bossInfo = m_bossInfo[pBoss->bossType - 1];
    m_pWorld->SendOnAddNotificationToAll(bossInfo.endImage, bossInfo.endNotifyMsg, bossInfo.endNotifyMsg, false);
}
