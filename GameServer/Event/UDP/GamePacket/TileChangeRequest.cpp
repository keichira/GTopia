#include "TileChangeRequest.h"
#include "Item/ItemInfoManager.h"
#include "../../../Player/Dialog/PlayerDialog.h"

void TileChangeRequest::HandleConsumable(GamePlayer* pPlayer, World* pWorld, GameUpdatePacket* pPacket)
{

}

void TileChangeRequest::Execute(GamePlayer* pPlayer, World* pWorld, GameUpdatePacket* pPacket)
{
    if(!pPlayer || !pWorld || !pPacket) {
        return;
    }

    /*if(pPacket->itemID != ITEM_ID_FIST) {
        
    }*/

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pPacket->itemID);
    if(!pItem) {
        return;
    }

    if(pItem->type == ITEM_TYPE_CLOTHES) {

        return;
    }

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(pPacket->tileX, pPacket->tileY);
    if(!pTile) {
        LOGGER_LOG_DEBUG("Unable to get tile %d %d for tile change request", pPacket->tileX, pPacket->tileY);
        return;
    }
    Vector2Int tilePos = pTile->GetPos();

    if(pItem->type == ITEM_TYPE_CONSUMABLE) {
        return;
    }

    if(pItem->type == ITEM_TYPE_WRENCH) {
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

    if(pTileItem->HasFlag(ITEM_FLAG_MOD) && !pPlayer->GetRole()->HasPerm(ROLE_PERM_USE_ITEM_TYPE_MOD)) {
        pPlayer->SendFakePingReply();

        if(pPacket->itemID == ITEM_ID_FIST) {
            if(IsMainDoor(pTileItem->id)) {
                pPlayer->SendOnTalkBubble("`w(stand over and punch to use)", true);
            }
            else {
                pPlayer->SendOnTalkBubble("`wIt's too strong to break.", true);
            }
        }
        else if(pItem->IsBackground()) {
            pPlayer->SendOnTalkBubble("`wCan't put anything behind that!", true);
        }

        pPlayer->PlaySFX("cant_break_tile.wav");
        return;
    }

    if(pPacket->itemID == ITEM_ID_FIST) {
        if(pPlayer->GetInventory().GetClothByPart(BODY_PART_HAND) == ITEM_ID_FIRE_HOSE) {
            if(pTile->HasFlag(TILE_FLAG_ON_FIRE)) {
                pTile->RemoveFlag(TILE_FLAG_ON_FIRE);
                pWorld->SendParticleEffectToAll(tilePos.x * 32, tilePos.y * 32, 149);
                pWorld->SendTileUpdate(tilePos.x, tilePos.y);
                return;
            }

            pPlayer->SendFakePingReply();
            return;
        }
        else {
            pTile->PunchTile((uint8)pPlayer->GetCharData().GetPunchDamage());

            float tileHealth = pTile->GetHealthPercent();
            if(tileHealth > 0) {
                if(pTileItem->type == ITEM_TYPE_WEATHER_MACHINE && pTileItem->weatherID != 200) {
                    if(pTile->HasFlag(TILE_FLAG_IS_ON)) {
                        if(pWorld->GetCurrentWeather() != pTileItem->weatherID) {
                            pWorld->SetCurrentWeather(pTileItem->weatherID);
                        }
                        else {
                            pWorld->SetCurrentWeather(pWorld->GetDefaultWeather());
                        }
                    }
                    else {
                        if(pWorld->GetCurrentWeather() == pTileItem->weatherID) {
                            pWorld->SetCurrentWeather(pWorld->GetDefaultWeather());
                        }
                        else {
                            pWorld->SetCurrentWeather(pTileItem->weatherID);
                        }
                    }
    
                    pTile->ToggleFlag(TILE_FLAG_IS_ON);
                    pWorld->SendCurrentWeatherToAll();
                }
    
                pWorld->SendTileApplyDamage(tilePos.x, tilePos.y, (int32)pPlayer->GetCharData().GetPunchDamage(), pPlayer->GetNetID());
                return;
            }
        }
    }

    if(pPacket->itemID != ITEM_ID_FIST) {
        if(pPacket->itemID == ITEM_ID_GUARDIAN_PINEAPPLE && pWorld->GetTileManager()->GetTile(KEY_TILE_GUARD_PINEAPPLE)) {
            pPlayer->SendFakePingReply();
            pPlayer->SendOnTalkBubble("This world already has a Guardian Pineapple somewhere on it, installing two would be dangerous!", true);
            return;
        }

        pPlayer->GetInventory().RemoveItem(pPacket->itemID, 1, pPlayer);
    }

    if(pPacket->itemID == ITEM_ID_FIST && pTileItem->HasFlag(ITEM_FLAG_AUTOPICKUP)) {
        if(!pPlayer->GetInventory().HaveRoomForItem(pTileItem->id, 1)) {
            pPlayer->SendFakePingReply();
            pPlayer->SendOnTalkBubble("I better not break that, I have no room to pick it up!", true);
            return;
        }
    }

    pWorld->HandleTilePackets(pPacket);

    if(pPacket->itemID == ITEM_ID_FIST && pTileItem->HasFlag(ITEM_FLAG_AUTOPICKUP)) {
        pPlayer->GetInventory().AddItem(pTileItem->id, 1, pPlayer);
    }
    
    if(pPacket->itemID == ITEM_ID_FIST) {
        pWorld->SendTileApplyDamage(tilePos.x, tilePos.y, (int32)pPlayer->GetCharData().GetPunchDamage(), pPlayer->GetNetID());
    }

    pWorld->SendTileUpdate(tilePos.x, tilePos.y);
}