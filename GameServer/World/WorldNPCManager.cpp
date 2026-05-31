#include "WorldNPCManager.h"
#include "World.h"
#include "../Context.h"
#include "Math/Math.h"
#include "../Player/PlayerManager.h"

WorldNPCManager::WorldNPCManager(World* pWorld)
: m_pWorld(pWorld), m_lastIndex(0), m_activeNpcCount(0)
{
    m_neutronBeams.resize(60);
}

WorldNPCManager::~WorldNPCManager()
{
}

bool WorldNPCManager::SpawnGhost(eNPCType ghostType, int32 x, int32 y, bool forceSpawn)
{
    if(!forceSpawn)
    {
        if(!m_pWorld)
            return false;

        TileInfo* pCharmTile = m_pWorld->GetTileManager()->GetKeyTile(KEY_TILE_GHOST_CHARM);
        if(pCharmTile && pCharmTile->HasFlag(TILE_FLAG_IS_ON))
            return false;
    }

    Vector2Float pos((x * 32) + 16, (y * 32) + 16);
    return Spawn(ghostType, pos, pos, 0, 0, 0);
}

bool WorldNPCManager::SpawnGhostTrap(GamePlayer* pPlayer, int32 x, int32 y)
{
    if(!pPlayer || !m_pWorld)
        return false;

    TileInfo* pTile = m_pWorld->GetTileManager()->GetTile(x, y);
    if(!pTile)
        return false;

    if(pTile->GetFG() != ITEM_ID_BLANK)
    {
        pPlayer->SendOnTalkBubble("There's no room for a jar there.", false);
        return false;
    }

    TileInfo* pBottom = m_pWorld->GetTileManager()->GetTile(x, y + 1);
    if(!pBottom)
        return false;

    if(!m_pWorld->IsTileCollidableForPlayer(pPlayer, pBottom, false))
    {
        pPlayer->SendOnTalkBubble("Put the jar on the ground!", false);
        return false;
    }

    Vector2Float pos((x * 32) + 16, (y * 32) + 16);
    if(Spawn(NPC_TYPE_GHOST_JAR, pos, Vector2Float(0, 0), 0, pPlayer->GetNetID(), 0))
    {
        m_pWorld->ThrowItemToPositionFromPlayer(pPlayer, pos, ITEM_ID_GHOST_JAR, 1);
        return true;
    }
    else
    {
        pPlayer->SendOnTalkBubble("This world is too busy to place a Ghost Jar", false);
        return false;
    }
}

void WorldNPCManager::OnNeutronBeam(GamePlayer* pPlayer, const Vector2Float& vStartPos, const Vector2Float& vEndPos)
{
    if(!pPlayer)
        return;

    int32 targetIdx = -1;

    for(uint32 i = 0; i < m_neutronBeams.size(); ++i)
    {
        NeutronBeamInfo& beam = m_neutronBeams[i];
        if(beam.lifeTime < 1)
        {
            beam.lifeTime = 500;
            beam.startPos = vStartPos;
            beam.endPos = vEndPos;
            beam.ownerNetID = pPlayer->GetNetID();
            beam.length = DistanceBetweenPoints(vStartPos, vEndPos);

            targetIdx = i;
            break;
        }
    }

    if(targetIdx == -1)
        return;

    /**
     * cross beam check
     * but first implement death things
     */
}

void WorldNPCManager::Update()
{
    if(!m_pWorld)
        return;

    uint64 deltaMS = m_lastUpdateTime.GetElapsedTime();
    if(deltaMS > 2000)
        deltaMS = 2000;

    if(deltaMS <= 100)
        return;

    m_lastUpdateTime.Reset();

    for(auto& npc : m_npcs)
    {
        if(npc.type == NPC_TYPE_NONE)
            continue;

        npc.Update(this, deltaMS);
    }

    for(auto& beam : m_neutronBeams)
    {
        if(beam.lifeTime > 0)
        {
            beam.lifeTime = (beam.lifeTime > deltaMS) ? (beam.lifeTime - deltaMS) : 0;
        }
    }
}

void WorldNPCManager::RemoveNpc(int32 index)
{
    if(index < 0 || index >= m_npcs.size())
        return;

    WorldNPC& npc = m_npcs[index];
    
    if(npc.type != NPC_TYPE_NONE)
    {
        npc.type = NPC_TYPE_NONE;

        if(m_activeNpcCount != 0)
        {
            m_activeNpcCount--;
        }
    }
}

WorldNPC* WorldNPCManager::GetNPCByID(int32 index)
{
    if(index < 0 || index >= m_npcs.size())
        return nullptr;

    WorldNPC* pNpc = &m_npcs[index];
    if(pNpc && pNpc->type == NPC_TYPE_NONE)
        return nullptr;

    return pNpc;
}

void WorldNPCManager::SuckGhostToTrap(WorldNPC* pTrap, float trapWidth, float trapHeight)
{
    if(!pTrap)
        return;

    for(auto& npc : m_npcs)
    {
        if(!npc.IsGhost() || npc.val1 == 1)
            continue;
        
        if(!npc.IsInside(pTrap, trapWidth, trapHeight))
            continue;

        npc.speed = 100.0f;
        npc.val1 = 1;
        npc.val2 = pTrap->id;
        npc.dest = pTrap->pos;
        
        m_pWorld->SendNPCPacketToAll(NPC_EVENT_MOVE, npc.id, npc.type, npc.pos, npc.dest, npc.speed, npc.val1, npc.val2);
    }
}

bool WorldNPCManager::IsGhostOnBeam(WorldNPC* pGhost)
{
    if(!pGhost || !pGhost->IsGhost())
        return false;

    float gLeft = pGhost->pos.x - 16.0f;
    float gTop = pGhost->pos.y - 16.0f;
    float gRight = pGhost->pos.x + 16.0f;
    float gBottom = pGhost->pos.y + 16.0f;

    if(pGhost->type == NPC_TYPE_BOSS_GHOST)
    {
        gLeft = pGhost->pos.x - 64.0f;
        gTop = pGhost->pos.y - 64.0f;
        gRight = pGhost->pos.x + 64.0f;
        gBottom = pGhost->pos.y + 64.0f;
    }

    RectFloat ghostRect(gLeft, gTop, gRight, gBottom);
    int32 activeBeamCount = 0;

    PlayerManager* pPlayerMgr = GetPlayerManager();

    for(auto& beam : m_neutronBeams)
    {
        if(beam.lifeTime > 0)
        {
            float minX = Min(beam.startPos.x, beam.endPos.x);
            float minY = Min(beam.startPos.y, beam.endPos.y);
            float maxX = Max(beam.startPos.x, beam.endPos.x);
            float maxY = Max(beam.startPos.y, beam.endPos.y);
            
            RectFloat beamRect(minX, minY, maxX, maxY);

            if(!ghostRect.Intersects(beamRect))
                continue;

            int32 step = (int32)(beam.length / 16.0f);
            if(step < 1) step = 1;

            float stepX = (beam.endPos.x - beam.startPos.x) / step;
            float stepY = (beam.endPos.y - beam.startPos.y) / step;

            Vector2Float currSamplePoint = beam.startPos;

            for(int32 i = 0; i < step; ++i)
            {
                if(ghostRect.IsInside(currSamplePoint))
                {
                    m_lastBeamPos = currSamplePoint;
            
                    if(pGhost->type != NPC_TYPE_BOSS_GHOST)
                        return true;
            
                    activeBeamCount++;
                    pGhost->beams.push_back(currSamplePoint);

                    GamePlayer* pPlayer = pPlayerMgr->GetPlayerByNetID(beam.ownerNetID);
                    if(pPlayer)
                    {
                        pGhost->OnGotHit(pPlayer, currSamplePoint, beam.startPos, this);
                    }
                    break;
                }
            
                currSamplePoint.x += stepX;
                currSamplePoint.y += stepY;
            }
        }
    }

    if(pGhost->type == NPC_TYPE_BOSS_GHOST && activeBeamCount > 0)
        return true;

    return false;
}

bool WorldNPCManager::Spawn(eNPCType npcType, const Vector2Float& pos, const Vector2Float& dest, int32 val1, int32 val2, float speed)
{
    if(GetContext()->GetGameConfig()->maxNpcPerWorld <= m_activeNpcCount)
        return false;

    WorldNPC* pNpc = nullptr;
    uint32 index = 0;

    for(uint32 i = 0; i < m_npcs.size(); ++i)
    {
        WorldNPC* pCandidate = &m_npcs[i];

        if(pCandidate && pCandidate->type == NPC_TYPE_NONE)
        {
            pNpc = pCandidate;
            index = i;
            break;
        }
    }

    if(!pNpc)
    {
        m_npcs.push_back(WorldNPC());
        pNpc = &m_npcs.back();
        index = m_npcs.size() - 1;
    }

    pNpc->Init(npcType, index, pos);
    pNpc->speed = speed;
    pNpc->val1 = val1;
    pNpc->val2 = val2;
    pNpc->dest = dest;

    m_pWorld->SendNPCPacketToAll(NPC_EVENT_ADD, index, npcType, pos, pNpc->dest, pNpc->speed, pNpc->val1, pNpc->val2);

    m_activeNpcCount++;
    m_lastIndex = index;

    return true;
}
