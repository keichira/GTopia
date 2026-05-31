#include "WorldNPC.h"
#include "WorldNPCManager.h"
#include "Math/Math.h"
#include "Math/Random.h"
#include "World.h"
#include "../Player/PlayerManager.h"

WorldNPC::WorldNPC()
: id(0), type(0), speed(0), val1(0), val2(0), lassoed(false), hp(0),
isInsidePowerNodes(false)
{
}

WorldNPC::~WorldNPC()
{
}

void WorldNPC::Init(eNPCType npcType, uint8 npcID, const Vector2Float& npcPos)
{
    type = npcType;
    id = npcID;
    pos = npcPos;
    dest = npcPos;
    speed = 0;
    val1 = 0;
    val2 = 0;
}

void WorldNPC::Update(WorldNPCManager* pNPCMgr, uint64 deltaMS)
{
    if(!pNPCMgr)
        return;

    World* pWorld = pNPCMgr->GetWorld();
    if(!pWorld)
        return;

    float deltaSec = deltaMS / 1000.0f;

    switch(type)
    {
        case NPC_TYPE_GHOST_JAR:
        {
            speed += deltaSec;
            if(val1 == 0) // waiting for jar to open
            {
                if(speed >= 2.0f)
                {
                    speed -= 2.0f;
                    val1 = 1;
                }
            }
            else
            {
                if(speed >= 5.0f)
                {
                    pNPCMgr->RemoveNpc(id);
                    return;
                }

                pNPCMgr->SuckGhostToTrap(this, 64, 96);
            }

            break;
        }

        case NPC_TYPE_BOSS_GHOST:
        {
            CheckGhostCanSlime(pNPCMgr);

            if(hp < 1)
            {
                pWorld->GetBossManager()->EndBoss(id);
                return;
            }

            if(nextAttackTimer.IsPassed())
            {
                val2 = (isInsidePowerNodes) ? 1000 : 0;
                nextAttackTimer.Set(500);
            }

            if(val1 == 0)
            {
                float moveAngle = Atan2(dest.y - pos.y, dest.x - pos.x);
                float distToDest = DistanceBetweenPoints(pos, dest);

                if(pos == dest || (distToDest <= speed * deltaSec)) // arrived
                {
                    pos = dest;
                    val1 = 1;
                    val2 = 800;
                    nextAttackTimer.Set(500);

                    speed = RandomRangeFloat(20.0f, 50.0f);

                    if(!isInsidePowerNodes)
                    {
                        MoveGhostRandom(pNPCMgr);
                    }
                    else
                    {
                        val2 = 1000;

                        TileInfo* pClosestNode = pWorld->GetTileManager()->GetClosestPowerNodeFromWorldPos(pos);
                        if(pClosestNode)
                        {
                            dest = pClosestNode->GetWorldPosCenter();
                        }
                        else
                        {
                            MoveGhostRandom(pNPCMgr);
                        }
                    }

                    pWorld->SendNPCPacketToAll(NPC_EVENT_MOVE, id, type, pos, dest, speed, val1, val2);
                }
                else
                {
                    pos.x += Cos(moveAngle) * speed * deltaSec;
                    pos.y += Sin(moveAngle) * speed * deltaSec;

                    if((val2 == 500 && nextAttackTimer.GetRemainingTime() > 99) || !pNPCMgr->IsGhostOnBeam(this) || beams.empty())
                    {
                        if(isInsidePowerNodes && (val2 != 1000 || nextAttackTimer.GetRemainingTime() < 100))
                        {
                            val2 = 1000;
                            nextAttackTimer.Set(500);
                            pWorld->SendNPCPacketToAll(NPC_EVENT_MOVE, id, type, pos, dest, speed, val1, val2);
                        }
                    }
                    else
                    {
                        val2 = 500;
                        nextAttackTimer.Set(500);
                        val1 = 0;

                        Vector2Float avgPos;

                        uint32 beamCount = beams.size();

                        for(auto& beamPos : beams)
                            avgPos += beamPos;

                        avgPos /= beamCount;
                        beams.clear();

                        float scatterAngle = Atan2(avgPos.y - pos.y, avgPos.x - pos.x);
    
                        float scatterX = Cos(scatterAngle) * 64.0f + pos.x;
                        dest.x = scatterX + RandomRangeFloat(-32.0f, 32.0f);
                        
                        float scatterY = Sin(scatterAngle) * 64.0f + pos.y;
                        dest.y = scatterY + RandomRangeFloat(-32.0f, 32.0f);
    
                        Vector2Int& vWorldSize = pWorld->GetTileManager()->GetSize();
    
                        float maxMapX = (vWorldSize.x - 1) * 32.0f;
                        float maxMapY = (vWorldSize.y - 1) * 32.0f;
    
                        dest.x = Clamp(dest.x, 0.0f, maxMapX);
                        dest.y = Clamp(dest.y, 0.0f, maxMapY);
    
                        speed = beamCount * 25.0f;
                        pWorld->SendNPCPacketToAll(NPC_EVENT_MOVE, id, type, pos, dest, speed, val1, val2);
                    }
                }
            }
            else
            {
                if(!isInsidePowerNodes)
                {

                }
                else if(val2 == 1000 && nextAttackTimer.GetRemainingTime() > 99)
                {
                    TileInfo* pClosestNode = pWorld->GetTileManager()->GetClosestPowerNodeFromWorldPos(pos);
                    if(pClosestNode && DistanceBetweenPoints(pClosestNode->GetWorldPos(), pos) < 32 * 5)
                    {
                        val2 = 800;
                        nextAttackTimer.Set(800);

                        pWorld->DestroyTileAndSendToAll(pClosestNode);
                        pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_SHRAPNEL_BOOM, pClosestNode->GetWorldPosCenter());
                        isInsidePowerNodes = false;
                    }
                }
                else
                {
                    val2 = 1000;
                    nextAttackTimer.Reset();
                    pWorld->SendNPCPacketToAll(NPC_EVENT_MOVE, id, type, pos, dest, speed, val1, val2);
                }

                val1 = 0;
            }
            
            break;
        }

        case NPC_TYPE_MIND_CONTROL_GHOST:
        case NPC_TYPE_GHOST:
        {
            CheckGhostCanSlime(pNPCMgr);
            
            if(val1 == 0 && val2 > 0)
            {
                val2 -= deltaMS;
            }

            if(val1 == 1)
            {
                WorldNPC* pTrap = pNPCMgr->GetNPCByID(val2);
                if(!pTrap || (pTrap && !pTrap->IsGhostTrap()))
                {
                    val1 = 0;
                    val2 = 0;
                }
            }

            bool shouldProcessMovement = true;

            if(val1 == 1)
            {
                WorldNPC* pTrap = pNPCMgr->GetNPCByID(val2);
                bool validTrap = false;
            
                if(pTrap && pTrap->IsGhostTrap())
                {
                    if(pTrap->val1 == 1)
                    {
                        if(DistanceBetweenPoints(pTrap->pos, dest) <= 2.0f)
                        {
                            validTrap = true;
                        }
                    }
                }
            
                if(!validTrap)
                {
                    val1 = 0;
                    val2 = 0;
            
                    MoveGhostRandom(pNPCMgr);
                    speed = RandomRangeFloat(20.0f, 50.0f);
            
                    if(type == NPC_TYPE_BOSS_GHOST)
                    {
                        speed *= 2.5f;
                    }
            
                    pWorld->SendNPCPacketToAll(NPC_EVENT_MOVE, id, type, pos, dest, speed, val1, val2);
                }
            }

            float moveAngle = Atan2(dest.y - pos.y, dest.x - pos.x);
            float distToDest = DistanceBetweenPoints(pos, dest);

            if(shouldProcessMovement && (val1 == 0 || val1 == 1))
            {
                if(pos == dest || (distToDest <= speed * deltaSec)) // arrived
                {
                    pos = dest;
    
                    if(val1 == 1) // trapped
                    {
                        pWorld->SendNPCPacketToAll(NPC_EVENT_SUCKED, id, val2, pos, dest, speed, val1, val2);
    
                        WorldNPC* pTrap = pNPCMgr->GetNPCByID(val2);
                        if(pTrap)
                        {
                            GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(pTrap->val2);
                            if(pPlayer)
                            {
                                int32 rewardItemID = ITEM_ID_GHOST_IN_A_JAR;
                                if(type == NPC_TYPE_HARVEST_GHOST) rewardItemID = ITEM_ID_ANCESTOR_MOONCAKE;
                                else if(type == NPC_TYPE_MIND_CONTROL_GHOST) rewardItemID = ITEM_ID_MIND_GHOST_IN_A_JAR;
        
                                if(type == NPC_TYPE_HARVEST_GHOST)
                                {
                                    pPlayer->SendOnTalkBubble("`9The ghost became an Acestor Mooncake!``", false);
                                }
                                else
                                {
                                    pPlayer->SendOnTalkBubble("`3I caught a ghost!``", false);
                                }
    
                                if(pPlayer->GetInventory().GetClothByPart(BODY_PART_CHESTITEM) == ITEM_ID_GHOST_DRAGON_CHARM)
                                {
                                    pPlayer->GiveXP(200);
                                }
    
                                uint8 fitCount = pPlayer->GetInventory().GetFitItemCount(rewardItemID);
                                if(fitCount == 0)
                                {
                                    TileInfo* pTile = pWorld->GetTileManager()->GetTileByWorldPos(pPlayer->GetWorldPos());
                                    if(pTile)
                                    {
                                        pWorld->DropObjectOnTile(pTile, rewardItemID, 1, Vector2Float(0, 0), true);
                                    }
                                }
                                else
                                {
                                    pPlayer->ModifyInventoryItem(rewardItemID, 1);
                                }
    
                                pWorld->ThrowItemToPlayerFromPosition(pPlayer, pTrap->pos, rewardItemID, 1);
                            }
    
                            pNPCMgr->RemoveNpc(pTrap->id);
                        }
    
                        pNPCMgr->RemoveNpc(id);
                    }
                    else
                    {
                        speed = RandomRangeFloat(20.0f, 50.0f);
                        MoveGhostRandom(pNPCMgr);
    
                        pWorld->SendNPCPacketToAll(NPC_EVENT_MOVE, id, type, pos, dest, speed, val1, val2);
                    }
                }
                else
                {
                    pos.x += Cos(moveAngle) * speed * deltaSec;
                    pos.y += Sin(moveAngle) * speed * deltaSec;
    
                    if(val2 < 1 && pNPCMgr->IsGhostOnBeam(this))
                    {
                        val2 = 500;
                        val1 = 0;
                        
                        Vector2Float& vLastLassoe = pNPCMgr->GetLastLassoePos();
                        float scatterAngle = Atan2(vLastLassoe.y - pos.y, vLastLassoe.x - pos.x);
    
                        float scatterX = Cos(scatterAngle) * 64.0f + pos.x;
                        dest.x = scatterX + RandomRangeFloat(-32.0f, 32.0f);
                        
                        float scatterY = Sin(scatterAngle) * 64.0f + pos.y;
                        dest.y = scatterY + RandomRangeFloat(-32.0f, 32.0f);
    
                        Vector2Int& vWorldSize = pWorld->GetTileManager()->GetSize();
    
                        float maxMapX = (vWorldSize.x - 1) * 32.0f;
                        float maxMapY = (vWorldSize.y - 1) * 32.0f;
    
                        dest.x = Clamp(dest.x, 0.0f, maxMapX);
                        dest.y = Clamp(dest.y, 0.0f, maxMapY);
    
                        speed = 30.0f;
                        pWorld->SendNPCPacketToAll(NPC_EVENT_MOVE, id, type, pos, dest, speed, val1, val2);
                    }
                }
            }

            break;
        }
    }
}

void WorldNPC::MoveGhostRandom(WorldNPCManager* pNPCMgr)
{
    if(!pNPCMgr)
        return;

    World* pWorld = pNPCMgr->GetWorld();
    if(!pWorld)
        return; 

    dest.x = pos.x + RandomRangeFloat(-128.0f, 128.0f);
    dest.y = pos.y + RandomRangeFloat(-128.0f, 128.0f);

    if(type != NPC_TYPE_BOSS_GHOST)
    {
        for(uint32 i = 0; i < pNPCMgr->GetNPCActiveOrNotCount(); ++i)
        {
            WorldNPC* pTrap = pNPCMgr->GetNPCByID(i);
            if(!pTrap)
                continue;

            if(!pTrap->IsGhostTrap())
                continue;
            
            if(DistanceBetweenPoints(dest, pTrap->pos) < 128.0f)
            {
                float escapeAngle = Atan2(pos.y - pTrap->pos.y, pos.x - pTrap->pos.x);
                
                dest.x = (Cos(escapeAngle) * 64.0f + pos.x) + RandomRangeFloat(-32.0f, 32.0f);
                dest.y = (Sin(escapeAngle) * 64.0f + pos.y) + RandomRangeFloat(-32.0f, 32.0f);
                speed = 50.0f;
                break;
            }
        }
    }

    if(dest.x < 32.0f) dest.x = 32.0f;
    if(dest.y < 32.0f) dest.y = 32.0f;

    Vector2Int& vWorldSize = pWorld->GetTileManager()->GetSize();
    float maxMapX = (vWorldSize.x - 1) * 32.0f;
    float maxMapY = (vWorldSize.y - 1) * 32.0f;

    if(type == NPC_TYPE_BOSS_GHOST)
    {
        maxMapX = (vWorldSize.x - 6) * 32.0f;
        maxMapY = (vWorldSize.y - 6) * 32.0f;
    }

    if(maxMapX < dest.x) dest.x = maxMapX;
    if(maxMapY < dest.y) dest.y = maxMapY;
}

void WorldNPC::CheckGhostCanSlime(WorldNPCManager* pNPCMgr)
{
    if(!pNPCMgr)
        return;

    World* pWorld = pNPCMgr->GetWorld();
    if(!pWorld)
        return;

    float halfSize = (type == NPC_TYPE_BOSS_GHOST) ? 64.0f : 16.0f;
    RectFloat ghostRect(pos.x - halfSize, pos.y - halfSize, pos.x + halfSize, pos.y + halfSize);

    auto playersInRect = pWorld->GetPlayersInWorldRect(ghostRect);
    if(playersInRect.empty())
        return;

    bool hitAnyPlayer = false;

    for(auto& pPlayer : playersInRect)
    {
        if(!pPlayer)
            continue;

        TileInfo* pTile = pWorld->GetTileManager()->GetTileByWorldPos(pPlayer->GetWorldPosCenter());
        if(pTile)
        {
            if(IsMainDoor(pTile->GetFG()))
                continue;
        }

        int32 hatItemID = pPlayer->GetInventory().GetClothByPart(BODY_PART_HAT);

        if(type == NPC_TYPE_GHOST_SHARK)
        {
            if(hatItemID != ITEM_ID_GHOSTKINGS_GLORY)
            {
                if(RandomRangeInt(0, 5) < 3)
                {
                    pWorld->SendTalkBubbleAndConsoleToAll("`2CHOMP CHOMP``", false, pPlayer);
                }

                // kill player
                hitAnyPlayer = true;
                break;
            }
        }
        else if(type == NPC_TYPE_MIND_CONTROL_GHOST)
        {
            if(!pPlayer->GetModController().HasPlayMod(PLAYMOD_TYPE_MIND_CONTROL))
            {
                hitAnyPlayer = true;
            }

            if(hatItemID != ITEM_ID_FOIL_HAT && hatItemID != ITEM_ID_ALIEN_MIND_PROTECTOR && hatItemID != ITEM_ID_GHOSTKINGS_GLORY)
            {
                pPlayer->GetModController().AddPlayMod(PLAYMOD_TYPE_MIND_CONTROL);
            }

            if(hatItemID == ITEM_ID_ALIEN_MIND_PROTECTOR)
            {
                pPlayer->PlaySFX("ghost_shield_reflect.wav");
            }
        }
        else if(hatItemID != ITEM_ID_GHOSTKINGS_GLORY)
        {
            if(!pPlayer->GetModController().HasPlayMod(PLAYMOD_TYPE_SLIMED))
            {
                pPlayer->SendOnTalkBubble("`2AIYEE! A ghost!``", false);
                hitAnyPlayer = true;
            }

            pPlayer->GetModController().AddPlayMod(PLAYMOD_TYPE_SLIMED);
        }
    }

    if(hitAnyPlayer)
    {
        pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_ECTO_SPLAT, pos);
    }
}

bool WorldNPC::IsGhost()
{
    return type == NPC_TYPE_GHOST || type == NPC_TYPE_MIND_CONTROL_GHOST || type == NPC_TYPE_GHOST_SHARK ||
           type == NPC_TYPE_BOSS_GHOST;
}

bool WorldNPC::IsGhostTrap()
{
    return type == NPC_TYPE_GHOST_JAR;
}

bool WorldNPC::IsInside(WorldNPC* pNpc, float padX, float padY)
{
    if(!pNpc)
        return false;

    if(this->type == NPC_TYPE_BOSS_GHOST) 
        return false;

    if(pNpc->pos.y <= this->pos.y)
        return false;

    if(this->pos.y <= (pNpc->pos.y - padY))
        return false;

    float dynamicWidth = ((pNpc->pos.y - this->pos.y) * padX) / padY;

    if(this->pos.x <= (pNpc->pos.x - (dynamicWidth / 2.0f)))
        return false;

    if ((pNpc->pos.x + (dynamicWidth / 2.0f)) <= this->pos.x)
        return false;

    return true;
}

void WorldNPC::OnGotHit(GamePlayer* pPlayer, const Vector2Float& hitPos, const Vector2Float& attackPos, WorldNPCManager* pNPCMgr)
{
    if(!pPlayer || !pNPCMgr)
        return;

    World* pWorld = pNPCMgr->GetWorld();
    if(!pWorld)
        return;
    
    if(type == NPC_TYPE_BOSS_GHOST)
    {
        uint16 handItem = pPlayer->GetInventory().GetClothByPart(BODY_PART_HAND);

        if(handItem != ITEM_ID_NEUTRON_GUN && handItem != ITEM_ID_NEUTRON_POWER_GLOVE)
            return;

        if(pPlayer->GetInventory().GetClothByPart(BODY_PART_BACK) != ITEM_ID_NEUTRON_PACK)
            return;

        if(!isInsidePowerNodes)
            return;

        hp -= 10;

        pWorld->SendConsoleMessageToAll("[Boss Ghost HP = " + ToString(hp) + "]");
        return;
    }

    if(IsGhost() && type != NPC_TYPE_BOSS_GHOST)
    {
        if(pPlayer->GetInventory().GetClothByPart(BODY_PART_HAND) == ITEM_ID_NEUTRON_POWER_GLOVE)
        {
            pWorld->SendNPCPacketToAll(NPC_EVENT_DIE, id, type, pos, dest, speed, val1, val2);
            pNPCMgr->RemoveNpc(id);
            pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_BLACKHOLE, pos);
        }
    }
}
