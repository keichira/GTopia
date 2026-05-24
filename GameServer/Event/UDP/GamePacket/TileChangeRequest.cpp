#include "TileChangeRequest.h"
#include "Item/ItemInfoManager.h"
#include "../../../Player/Dialog/PlayerDialog.h"

void TileChangeRequest::Execute(GamePlayer* pPlayer, World* pWorld, GameUpdatePacket* pPacket)
{
    if(!pPlayer || !pWorld || !pPacket)
        return;

    Role* pRole = pPlayer->GetRole();
    if(!pRole)
        return;

    Timer& lastTileChangeTime = pPlayer->GetLastTileChangeTime();
    if(!lastTileChangeTime.IsPassed())
        return;

    lastTileChangeTime.Set(pPacket->itemID == ITEM_ID_FIST ? 130 : 70);

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pPacket->itemID);
    if(!pItem)
        return;

    if(pItem->HasFlag(ITEM_FLAG_MOD) && !pRole->HasPerm(ROLE_PERM_MSTATE))
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(pPacket->tileX, pPacket->tileY);
    if(!pTile)
        return;

    TileInfo* pPlayerTile = pWorld->GetTileManager()->GetTileByWorldPos(pPlayer->GetWorldPos());
    if(!pPlayerTile)
        return;

    /*if(!pWorld->CanPlayerTravelStraight(pPlayer, pPlayerTile, pTile))
    {
        pPlayer->SendFakePingReply();
        return;
    }*/

    PlayerInventory& inventory = pPlayer->GetInventory();

    if(pPacket->itemID == ITEM_ID_FIST)
    {
        uint16 handItemID = inventory.GetClothByPart(BODY_PART_HAND);
        uint16 backItemID = inventory.GetClothByPart(BODY_PART_BACK);

        if(handItemID == ITEM_ID_PARTY_BUBBLE_BLASTER)
        {
            pPlayer->SendFakePingReply();
            return;
        }

        if((handItemID == ITEM_ID_CHAINSAW_HAND && pTile->GetFG() == ITEM_ID_ICE_SCULPTURES) ||
            (handItemID == ITEM_ID_GARDEN_SHEARS && (pTile->GetFG() == ITEM_ID_TOPIARY_HEDGE || pTile->GetFG() == ITEM_ID_SPOOKY_BUNTING))
        ) {
            if(!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
                return;

            if(pTile->HasFlag(TILE_FLAG_BG_IS_ON))
            {
                pTile->RemoveFlag(TILE_FLAG_BG_IS_ON);

                if(pTile->HasFlag(TILE_FLAG_FG_ALT_MODE))
                    pTile->RemoveFlag(TILE_FLAG_FG_ALT_MODE);
                else
                    pTile->SetFlag(TILE_FLAG_FG_ALT_MODE);
            }
            else
            {
                pTile->SetFlag(TILE_FLAG_BG_IS_ON);

                if(pTile->HasFlag(TILE_FLAG_FG_ALT_MODE))
                    pTile->SetFlag(TILE_FLAG_FG_ALT_MODE);
                else
                    pTile->RemoveFlag(TILE_FLAG_FG_ALT_MODE);
            }

            pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_SHRAPNEL_BOOM, pTile->GetWorldPosCenter());
            pWorld->SendTileUpdate(pTile);
            return;
        }

        if(IsFuelPack(backItemID) && handItemID != ITEM_ID_FLAMETHROWER)
        {
            pPlayer->SendFakePingReply();
            return;
        }

        if(handItemID == ITEM_ID_CLOAK_OF_FALLING_WATERS ||
            handItemID == ITEM_ID_SUPER_SQUIRT_RIFLE_500 ||
            handItemID == ITEM_ID_FIRE_HOSE
        ) {
            if(pTile->HasFlag(TILE_FLAG_ON_FIRE))
            {
                //put out fire update tile
            }
            else
            {
                pPlayer->SendFakePingReply();
            }

            return;
        }
    }

    if(pTile->GetFG() == ITEM_ID_RUNE_CARVED_DOOR && !pRole->HasPerm(ROLE_PERM_MSTATE))
    {
        pPlayer->SendFakePingReply();
        return;
    }

    if(pTile->HasFlag(TILE_FLAG_ON_FIRE) && 
        (pPacket->itemID != ITEM_ID_WATER_BALLOON &&
        pPacket->itemID != ITEM_ID_WATER_BUCKET &&
        pPacket->itemID != ITEM_ID_SMALL_WAR_BALLOON &&
        pPacket->itemID != ITEM_ID_MEDIUM_WAR_BALLOON &&
        pPacket->itemID != ITEM_ID_LARGE_WAR_BALLOON)
    ) {
        pPlayer->SendFakePingReply();
        return;
    }

    ItemInfo* pTileItem = GetItemInfoManager()->GetItemByID(pTile->GetDisplayedItem());
    if(!pTileItem)
        return;

    if(pTileItem->type == ITEM_TYPE_LOCK)
    {
        TileExtra_Lock* pTileExtraLock = pTile->GetExtra<TileExtra_Lock>();
        if(!pTileExtraLock)
            return;

        if(pTileExtraLock->ownerID != pPlayer->GetUserID())
        {
            pWorld->OnPunchedLock(pPlayer, pTile, pTileItem);
            return;
        }
    }

    if(pItem->type == ITEM_TYPE_CONSUMABLE)
        return;

    if(pItem->type == ITEM_TYPE_CLOTHES)
        return;

    bool tileBroken = false;

    if(pPacket->itemID != ITEM_ID_FIST)
    {
        if(!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        {
            pPlayer->SendFakePingReply();
            return;
        }

        if(pItem->type == ITEM_TYPE_WRENCH)
        {
            PlayerDialog::Handle(pPlayer, pTile);
            return;
        }

        if(inventory.GetCountOfItem(pPacket->itemID) < 1)
            return;

        if(IsJammer(pPacket->itemID) ||
            pItem->type == ITEM_TYPE_WEATHER_MACHINE ||
            pItem->type == ITEM_TYPE_WEATHER_SPECIAL2 ||
            pItem->type == ITEM_TYPE_INFINITY_WEATHER_MACHINE
        ) {
            TileInfo* pLockTile = pWorld->GetTileManager()->GetKeyTile(KEY_TILE_WORLD_LOCK);

            if(!pLockTile)
            {
                pPlayer->SendOnTalkBubble("This item can't be used by strangers in locked areas that are marked public", true);
                return;
            }
        }

        if((pPacket->itemID == ITEM_ID_GUARDIAN_PINEAPPLE && pWorld->GetTileManager()->GetKeyTile(KEY_TILE_GUARD_PINEAPPLE))
            || (pPacket->itemID == ITEM_ID_PUNCH_JAMMER && pWorld->GetTileManager()->GetKeyTile(KEY_TILE_PUNCH_JAMMER))
            || (pPacket->itemID == ITEM_ID_ZOMBIE_JAMMER && pWorld->GetTileManager()->GetKeyTile(KEY_TILE_ZOMBIE_JAMMER))
            || (pPacket->itemID == ITEM_ID_SIGNAL_JAMMER && pWorld->GetTileManager()->GetKeyTile(KEY_TILE_SIGNAL_JAMMER))
            || (pPacket->itemID == ITEM_ID_ANTIGRAVITY_GENERATOR && pWorld->GetTileManager()->GetKeyTile(KEY_TILE_ANTIGRAVITY))
        ) {
            pPlayer->SendFakePingReply();
            pPlayer->SendOnTalkBubble("This world already has a " + pItem->name + " somewhere on it, installing two would be dangerous!", true);
            return;
        }

        if(pItem->type == ITEM_TYPE_LOCK)
        {
            pWorld->OnAddLock(pPlayer, pTile, pItem->id);
            return;
        }

        if(pItem->type == ITEM_TYPE_SEED)
        {
            pWorld->OnPlantSeed(pPlayer, pTile, pItem, pPacket);
            return;
        }

        pPlayer->ModifyInventoryItem(pPacket->itemID, -1);
        pWorld->HandleTilePackets(pPacket);

        pPlayer->GetProgressData().AddProgress(PLAYER_PROGRESS_PLACE_COUNT, 1);
        tileBroken = true;
    }
    else
    {
        if(pTile->GetDisplayedItem() == ITEM_ID_BLANK)
            return;

        if(!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
            return;

        if(pTileItem->HasFlag(ITEM_FLAG_SMOD) && !pRole->HasPerm(ROLE_PERM_SMSTATE))
            return;

        float tileHealthPercentBefore = pTile->GetHealthPercent();

        if(pTileItem->type == ITEM_TYPE_SEED)
        {
            if(pTile->GetGrowthPercent() >= 100.0f) {
                if(pTileItem->rarity < 999)
                {
                    pPlayer->GiveXP(pTileItem->rarity / 5 + 1);
                }

                pPlayer->GetProgressData().AddProgress(PLAYER_PROGRESS_HARVEST_COUNT, 1);
                pWorld->OnHarvestTree(pPlayer, pTile);
                return;
            }
        }

        if(pTileItem->type == ITEM_TYPE_PROVIDER)
        {

        }

        if(pTileItem->HasFlag(ITEM_FLAG_AUTOPICKUP))
        {
            if(!inventory.HaveRoomForItem(pTileItem->id, 1) && tileHealthPercentBefore < 1.0f)
            {
                pPlayer->SendOnTalkBubble("I better not break that, I have no room to pick it up!", true);
                return;
            }
        }

        uint32 punchDamage = pPlayer->GetCharData().GetPunchDamage();

        if(inventory.GetClothByPart(BODY_PART_HAND) == ITEM_ID_DIGGERS_SPADE)
        {
            bool validForDigger =
                pTileItem->id == ITEM_ID_DIRT ||
                pTileItem->id == ITEM_ID_CAVE_BACKGROUND;

            if(!validForDigger)
                return;

            pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_SHRAPNEL_BOOM, pTile->GetWorldPosCenter());
        }

        if(!pTile->WillBreak(punchDamage))
        {
            pPacket->type = NET_GAME_PACKET_TILE_APPLY_DAMAGE;
            pPacket->tileDamage = punchDamage;

            if(pTileItem->id == ITEM_ID_SIGNAL_JAMMER)
            {
                if(!pTile->HasFlag(TILE_FLAG_IS_ON))
                    pWorld->SendConsoleMessageToAll("Signal jammer enabled. This world is now `4hidden`` from the universe.");
                else
                    pWorld->SendConsoleMessageToAll("Signal jammer disabled.  This world is `2visible`` to the universe.");
            }

            if(pTileItem->id == ITEM_ID_ANTIGRAVITY_GENERATOR && tileHealthPercentBefore == 1.0f)
            {
                if(!pTile->HasFlag(TILE_FLAG_IS_ON))
                    pWorld->SendConsoleMessageToAll("`2Antigravity enabled!``");
                else
                    pWorld->SendConsoleMessageToAll("`4Antigravity disabled!``");
            }
        }
        else
        {
            if(pTileItem->HasFlag(ITEM_FLAG_AUTOPICKUP))
            {
                pWorld->ThrowItemToPlayerFromPosition(pPlayer, pTile->GetWorldPosCenter(), pTileItem->id, 1);
                pPlayer->ModifyInventoryItem(pTileItem->id, 1);
            }
            else
            {
                pWorld->OnTileDestroyedDropObject(pPlayer, pTile);
            }

            if(pTileItem->type != ITEM_TYPE_SEED)
            {
                pPlayer->GetProgressData().AddProgress(PLAYER_PROGRESS_BREAK_COUNT, 1);
            }

            if(pTileItem->rarity < 999 && !pTileItem->HasFlag(ITEM_FLAG_PERMANENT))
            {
                if(pTileItem->type != ITEM_TYPE_SEED)
                {
                    pPlayer->GiveXP(pTileItem->rarity / 5 + 1);
                }
            }

            if(pTileItem->type == ITEM_TYPE_LOCK)
            {
                pWorld->OnRemoveLock(pPlayer, pTile);
            }

            tileBroken = true;
        }

        pWorld->HandleTilePackets(pPacket);

        if(tileBroken && IsWorldLock(pTileItem->id))
        {
            pWorld->SendNameChangeToAll(pPlayer);
        }
    }

    if(pItem->id == ITEM_ID_FIST)
    {
        if(pTile->GetHealthPercent() > 0.0f)
        {
            pWorld->SendTileApplyDamage(pTile, pPlayer->GetCharData().GetPunchDamage(), pPlayer->GetNetID());
        }
        else if(tileBroken)
        {
            GameUpdatePacket packet;
            packet.type   = NET_GAME_PACKET_TILE_CHANGE_REQUEST;
            packet.netID  = pPlayer->GetNetID();
            packet.itemID = ITEM_ID_FIST;

            Vector2Int& vTilePos = pTile->GetPos();
            packet.tileX = vTilePos.x;
            packet.tileY = vTilePos.y;

            pWorld->SendGamePacketToAll(&packet);
        }
    }

    pWorld->SendTileUpdate(pTile);
}