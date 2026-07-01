#include "PlayerDialog.h"
#include "../GamePlayer.h"
#include "Item/ItemInfoManager.h"
#include "World/TileInfo.h"

#include "AchievementBlockDialog.h"
#include "BattleCageDialog.h"
#include "BulletinBlockDialog.h"
#include "CrystalBlockDialog.h"
#include "DonationBoxDialog.h"
#include "DoorDialog.h"
#include "LockDialog.h"
#include "MailboxBlockDialog.h"
#include "OuijaBoardDialog.h"
#include "SignDialog.h"
#include "XenoniteDialog.h"

void PlayerDialog::Handle(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pTile)
    {
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetDisplayedItem());
    if (!pItem)
        return;

    if (pItem->type == ITEM_TYPE_SIGN)
    {
        SignDialog::Request(pPlayer, pTile);
        return;
    }

    if (pItem->type == ITEM_TYPE_LOCK)
    {
        LockDialog::Request(pPlayer, pTile);
        return;
    }

    if (pItem->type == ITEM_TYPE_ACHIEVEMENT)
    {
        AchievementBlockDialog::Request(pPlayer, pTile, pItem);
        return;
    }

    if (pItem->type == ITEM_TYPE_OUIJA_BOARD)
    {
        OuijaBoardDialog::RequestMain(pPlayer, pTile);
        return;
    }

    if (pItem->type == ITEM_TYPE_BATTLE_CAGE)
    {
        BattleCageDialog::Request(pPlayer, pTile, pItem);
        return;
    }

    if (pItem->type == ITEM_TYPE_XENONITE)
    {
        XenoniteDialog::Request(pPlayer, pTile, pItem);
        return;
    }

    if (pItem->type == ITEM_TYPE_MAILBOX)
    {
        MailboxBlockDialog::Request(pPlayer, pTile, pItem);
        return;
    }

    if (pItem->type == ITEM_TYPE_CRYSTAL)
    {
        CrystalBlockDialog::Request(pPlayer, pTile, pItem);
        return;
    }

    if (pItem->type == ITEM_TYPE_BULLETIN)
    {
        BulletinBlockDialog::Request(pPlayer, pTile, pItem);
        return;
    }

    if (pTile->IsTileExtraType(TILE_EXTRA_TYPE_DOOR))
    {
        DoorDialog::Request(pPlayer, pTile, pItem);
        return;
    }

    if (pItem->type == ITEM_TYPE_DONATION_BOX)
    {
        DonationBoxDialog::Request(pPlayer, pTile, pItem);
        return;
    }
}
