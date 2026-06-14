#include "PlayerDialog.h"
#include "../GamePlayer.h"
#include "Item/ItemInfoManager.h"
#include "World/TileInfo.h"

#include "SignDialog.h"
#include "LockDialog.h"
#include "AchievementBlockDialog.h"
#include "OuijaBoardDialog.h"
#include "BattleCageDialog.h"
#include "XenoniteDialog.h"

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

    if(pItem->type == ITEM_TYPE_OUIJA_BOARD)
    {
        OuijaBoardDialog::RequestMain(pPlayer, pTile);
    }

    if(pItem->type == ITEM_TYPE_BATTLE_CAGE)
    {
        BattleCageDialog::Request(pPlayer, pTile, pItem);
    }

    if(pItem->type == ITEM_TYPE_XENONITE)
    {
        XenoniteDialog::Request(pPlayer, pTile, pItem);
    }
}
