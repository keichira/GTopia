#include "ObjectActivateRequest.h"
#include "Utils/GrowUtils.h"
#include "Math/Math.h"

void ObjectActivateRequest::Execute(GamePlayer* pPlayer, World* pWorld, GameUpdatePacket* pPacket)
{
    if(!pPlayer || !pWorld || !pPacket)
        return;

    WorldObject* pObject = pWorld->GetObjectManager()->GetObjectByID(pPacket->field_7);
    if(!pObject)
        return;

    if(IsIllegalItem(pObject->itemID) && !pPlayer->GetRole()->HasPerm(ROLE_PERM_BYPASS_ILLEGAl_ITEM))
        return;

    ItemInfo* pItemInfo = GetItemInfoManager()->GetItemByID(pObject->itemID);
    if(!pItemInfo)
        return;

    Role* pRole = pPlayer->GetRole();
    if(!pRole)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTileByWorldPos(pObject->GetCenterPos());
    if(!pTile)
        return;

    if(pPlayer->GetDistToTileInTiles(pTile) > 5)
        return;

    if(pWorld->IsTileCollidableForPlayer(pPlayer, pTile, false))
        return;

    if((pTile->GetDisplayedItem() == ITEM_ID_DISPLAY_BOX || pTile->GetDisplayedItem() == ITEM_ID_TRANSMATTER_FIELD) && !pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        return;

    TileInfo* pPlayerTile = pWorld->GetTileManager()->GetTileByWorldPos(pPlayer->GetWorldPos());
    if(!pPlayerTile)
        return;

    if(!pWorld->CanPlayerTravelToTile(pPlayer, pPlayerTile, pTile))
        return;

    if(pItemInfo->HasFlag(ITEM_FLAG_MOD) && !pRole->HasPerm(ROLE_PERM_MSTATE))
        return;

    PlayerInventory& inventory = pPlayer->GetInventory();

    uint32 fitCount = inventory.GetFitItemCount(pObject->itemID);
    if(fitCount == 0)
        return;

    uint32 takeCount = Min(pObject->count, fitCount);
    if(takeCount == 0)
        return;
    
    uint32 remaining = pObject->count - takeCount;
    
    if(pObject->itemID != ITEM_ID_GEMS && remaining > 0)
    {
        pWorld->DropObjectOnTile(pTile, pObject->itemID, remaining, Vector2Float(0, 0), false);
    }

    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_ITEM_CHANGE_OBJECT;
    packet.field_5 = -1;
    packet.field_4 = pPlayer->GetNetID();
    packet.field_7 = pObject->objectID;

    pWorld->HandleTilePackets(&packet);

    if(pItemInfo->id == ITEM_ID_GEMS)
    {
        pPlayer->ModifyGems(takeCount, false);
    }
    else
    {
        inventory.AddItem(pItemInfo->id, takeCount);

        string noticeMessage = "Collected `w" + ToString(takeCount) + " " + pItemInfo->name + "``.";
        if(pItemInfo->rarity != 999)
        {
            noticeMessage += " Rarity: `w" + ToString(pItemInfo->rarity) + "``.";
        }

        pPlayer->SendOnConsoleMessage(noticeMessage);
    }

    pWorld->SendGamePacketToAll(&packet);
}