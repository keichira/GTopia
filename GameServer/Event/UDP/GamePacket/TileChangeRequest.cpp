#include "TileChangeRequest.h"
#include "Item/ItemInfoManager.h"
#include "../../../Player/Dialog/PlayerDialog.h"

void TileChangeRequest::OnPunchedLock(GamePlayer* pPlayer, TileInfo* pTile)
{

}

void TileChangeRequest::HandleConsumable(GamePlayer *pPlayer, World *pWorld, GameUpdatePacket *pPacket)
{

}

void TileChangeRequest::Execute(GamePlayer* pPlayer, World* pWorld, GameUpdatePacket* pPacket)
{
    if(!pPlayer || !pWorld || !pPacket)
        return;


    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pPacket->itemID);
    if(!pItem)
        return;

    if(pItem->type == ITEM_TYPE_CLOTHES) 
    {

        return;
    }

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(pPacket->tileX, pPacket->tileY);
    if(!pTile) 
    {
        LOGGER_LOG_DEBUG("Unable to get tile %d %d for tile change request", pPacket->tileX, pPacket->tileY);
        return;
    }

    if(pItem->type == ITEM_TYPE_CONSUMABLE) 
    {
        return;
    }

    if(!pWorld->PlayerHasAccessOnTile(pPlayer, pTile)) 
    {
        pPlayer->SendFakePingReply();
        return;
    }

    Vector2Int tilePos = pTile->GetPos();

    if(pItem->type == ITEM_TYPE_WRENCH) 
    {
        PlayerDialog::Handle(pPlayer, pTile);
        return;
    }

    if(
        pTile->GetDisplayedItem() == ITEM_ID_BLANK && pPacket->itemID == ITEM_ID_FIST
        && pTile->HasFlag(TILE_FLAG_ON_FIRE)
    ) {
        pPlayer->SendFakePingReply();
        return;
    }

    ItemInfo* pTileItem = GetItemInfoManager()->GetItemByID(pTile->GetDisplayedItem());

    if(pPacket->itemID != ITEM_ID_FIST) 
    {
        if(
            (!pTileItem->IsBackground() && pTile->GetFG() != ITEM_ID_BLANK) ||
            (pTileItem->IsBackground() && pTile->GetBG() != ITEM_ID_BLANK)
        ) {
            return;
        }
    }

    if(pTileItem->HasFlag(ITEM_FLAG_MOD) && !pPlayer->GetRole()->HasPerm(ROLE_PERM_USE_ITEM_TYPE_MOD)) 
    {
        pPlayer->SendFakePingReply();

        if(pPacket->itemID == ITEM_ID_FIST) 
        {
            if(IsMainDoor(pTileItem->id)) 
            {
                pPlayer->SendOnTalkBubble("`w(stand over and punch to use)", true);
            }
            else 
            {
                pPlayer->SendOnTalkBubble("`wIt's too strong to break.", true);
            }
        }
        else if(pItem->IsBackground()) 
        {
            pPlayer->SendOnTalkBubble("`wCan't put anything behind that!", true);
        }

        pPlayer->PlaySFX("cant_break_tile.wav");
        return;
    }

    if(pPacket->itemID != ITEM_ID_FIST) 
    {
        if(
            pPacket->itemID == ITEM_ID_GUARDIAN_PINEAPPLE && pWorld->GetTileManager()->GetKeyTile(KEY_TILE_GUARD_PINEAPPLE) ||
            pPacket->itemID == ITEM_ID_PUNCH_JAMMER && pWorld->GetTileManager()->GetKeyTile(KEY_TILE_PUNCH_JAMMER) ||
            pPacket->itemID == ITEM_ID_ZOMBIE_JAMMER && pWorld->GetTileManager()->GetKeyTile(KEY_TILE_ZOMBIE_JAMMER) ||
            pPacket->itemID == ITEM_ID_SIGNAL_JAMMER && pWorld->GetTileManager()->GetKeyTile(KEY_TILE_SIGNAL_JAMMER) ||
            pPacket->itemID == ITEM_ID_ANTIGRAVITY_GENERATOR && pWorld->GetTileManager()->GetKeyTile(KEY_TILE_ANTIGRAVITY)
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

        pPlayer->GetProgressData().AddProgress(PLAYER_PROGRESS_PLACE_COUNT, 1);
        pPlayer->ModifyInventoryItem(pItem->id, -1);
    }

    if(pPacket->itemID == ITEM_ID_FIST) 
    {
        if(pPlayer->GetInventory().GetClothByPart(BODY_PART_HAND) == ITEM_ID_FIRE_HOSE) 
        {
            if(pTile->HasFlag(TILE_FLAG_ON_FIRE)) 
            {
                pTile->RemoveFlag(TILE_FLAG_ON_FIRE);
                pWorld->SendParticleEffectToAll(tilePos.x * 32, tilePos.y * 32, 149);
                pWorld->SendTileUpdate(tilePos.x, tilePos.y);
                return;
            }

            pPlayer->SendFakePingReply();
            return;
        }
        else 
        {
            pTile->PunchTile((uint8)pPlayer->GetCharData().GetPunchDamage());
            pPlayer->GetProgressData().AddProgress(PLAYER_PROGRESS_PUNCH_COUNT, 1);

            if(pTileItem->type == ITEM_TYPE_SEED)
            {
                TileExtra_Seed* pTileExtra = pTile->GetExtra<TileExtra_Seed>();
                if(
                    pTileExtra && 
                    GetTileExtraGrowthPercent(pTileItem->growTime, GetTileExtraGrowth(pTileExtra, pTileExtra->timer, pTileExtra->growTime)) >= 100.0f
                )
                {
                    pWorld->OnHarvestTree(pPlayer, pTile);
                    return;
                }
            }

            float tileHealth = pTile->GetHealthPercent();
            if(tileHealth > 0) 
            {
                if(pTileItem->type == ITEM_TYPE_WEATHER_MACHINE && pTileItem->weatherID != 200) 
                {
                    if(pTile->HasFlag(TILE_FLAG_IS_ON)) 
                    {
                        if(pWorld->GetCurrentWeather() != pTileItem->weatherID) 
                        {
                            pWorld->SetCurrentWeather(pTileItem->weatherID);
                        }
                        else 
                        {
                            pWorld->SetCurrentWeather(pWorld->GetDefaultWeather());
                        }
                    }
                    else 
                    {
                        if(pWorld->GetCurrentWeather() == pTileItem->weatherID) 
                        {
                            pWorld->SetCurrentWeather(pWorld->GetDefaultWeather());
                        }
                        else 
                        {
                            pWorld->SetCurrentWeather(pTileItem->weatherID);
                        }
                    }
    
                    pTile->ToggleFlag(TILE_FLAG_IS_ON);
                    pWorld->SendCurrentWeatherToAll();
                }
    
                pWorld->SendTileApplyDamage(tilePos.x, tilePos.y, (int32)pPlayer->GetCharData().GetPunchDamage(), pPlayer->GetNetID());
                return;
            }

            if(pWorld->GetTileManager()->GetKeyTile(KEY_TILE_GUARD_PINEAPPLE) && !pPlayer->GetInventory().HaveRoomForItem(pTileItem->id, 1)) 
            {
                pPlayer->SendFakePingReply();
                pPlayer->SendOnTalkBubble("I better not break that, I have no room to pick it up!", true);
                return;
            }
    
            if(pTileItem->HasFlag(ITEM_FLAG_AUTOPICKUP)) 
            {
                pPlayer->GetInventory().AddItem(pTileItem->id, 1, pPlayer);
            }
    
            if(pTileItem->type == ITEM_TYPE_LOCK) 
            {
                pWorld->OnRemoveLock(pPlayer, pTile);
            }
            
            if(pTileItem->type == ITEM_TYPE_WEATHER_MACHINE) 
            {
                pWorld->SetCurrentWeather(pWorld->GetDefaultWeather());
                pTile->RemoveFlag(TILE_FLAG_IS_ON);
                pWorld->SendCurrentWeatherToAll();
            }
    
            pPlayer->GetProgressData().AddProgress(PLAYER_PROGRESS_BREAK_COUNT, 1);
            pWorld->SendTileApplyDamage(tilePos.x, tilePos.y, (int32)pPlayer->GetCharData().GetPunchDamage(), pPlayer->GetNetID());
        }
    }

    pWorld->HandleTilePackets(pPacket);
    pWorld->SendTileUpdate(tilePos.x, tilePos.y);
}