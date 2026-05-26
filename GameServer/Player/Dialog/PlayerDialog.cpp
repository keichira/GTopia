#include "PlayerDialog.h"
#include "../GamePlayer.h"
#include "Item/ItemInfoManager.h"
#include "World/TileInfo.h"

#include "SignDialog.h"
#include "LockDialog.h"
#include "AchievementBlockDialog.h"

void PlayerDialog::Handle(GamePlayer* pPlayer, TileInfo* pTile)
{
    if(!pTile) {
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetDisplayedItem());
    if(!pItem)
        return;

    if(pItem->type == ITEM_TYPE_SIGN) 
    {
        SignDialog::Request(pPlayer, pTile);
    }

    if(pItem->type == ITEM_TYPE_LOCK) 
    {
        LockDialog::Request(pPlayer, pTile);
    }

    if(pItem->type == ITEM_TYPE_ACHIEVEMENT)
    {
        AchievementBlockDialog::Request(pPlayer, pTile, pItem);
    }
}
