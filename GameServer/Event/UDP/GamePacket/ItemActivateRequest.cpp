#include "ItemActivateRequest.h"
#include "Item/ItemInfoManager.h"

bool ConvertItem(uint16 convertThis, uint16 toThis) 
{
    ItemInfo* pConvItem = GetItemInfoManager()->GetItemByID(convertThis);
    if(!pConvItem)
        return false;

    ItemInfo* pTargetItem = GetItemInfoManager()->GetItemByID(toThis);
    if(!pTargetItem)
        return false;

    return true;
}

void ItemActivateRequest::Execute(GamePlayer* pPlayer, World* pWorld, GameUpdatePacket* pPacket)
{
    if(!pPlayer || !pWorld || !pPacket)
        return;

    if(!pPlayer->CanActivateItemNow())
        return;

    pPlayer->ResetItemActiveTime();

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pPacket->field_7);
    if(!pItem) 
    {
        LOGGER_LOG_WARN("Player %s (ID: %d) tried to activate item %d", pPlayer->GetRawName(), pPlayer->GetUserID(), pPacket->field_7);
        return;
    }

    if(pItem->HasFlag(ITEM_FLAG_MOD) && !pPlayer->GetRole()->HasPerm(ROLE_PERM_USE_ITEM_TYPE_MOD)) 
    {
        LOGGER_LOG_WARN("Player %s (ID: %d) tried to use mod flagged item %d", pPlayer->GetRawName(), pPlayer->GetUserID(), pPacket->field_7);
        return;
    }

    if(pItem->type == ITEM_TYPE_CLOTHES) 
    {
        pPlayer->ToggleCloth(pPacket->field_7);
        return;
    }
}