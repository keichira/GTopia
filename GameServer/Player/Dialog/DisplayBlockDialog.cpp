#include "DisplayBlockDialog.h"
#include "../../World/WorldManager.h"
#include "../GamePlayer.h"
#include "Item/ItemInfoManager.h"
#include "Utils/DialogBuilder.h"

void DisplayBlockDialog::Request(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    TileExtra_DisplayBlock* pTileExtra = pTile->GetExtra<TileExtra_DisplayBlock>();
    if (!pTileExtra)
        return;

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon("`wDisplay Block``", pTile->GetFG(), true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y);

    if (pTileExtra->itemID == ITEM_ID_BLANK)
    {
        db.AddTextBox("The Display Block is empty. Use an item on it to display the item!");
    }
    else
    {
        ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTileExtra->itemID);
        if (!pItem)
            return;

        db.AddTextBox("A " + pItem->name + " in on display here.");
    }

    if (pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
        db.EndDialog("displayblock", "Pick it up", "Leave it");
    else
        db.EndDialog("displayblock", "", "Okay");

    pPlayer->SendOnDialogRequest(db.Get());
}

void DisplayBlockDialog::RequestPutItem(GamePlayer* pPlayer, TileInfo* pTile, int32 itemID)
{
    if (!pPlayer || !pTile)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
        return;

    TileExtra_DisplayBlock* pTileExtra = pTile->GetExtra<TileExtra_DisplayBlock>();
    if (!pTileExtra)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    if (!pWorld->GetTileManager()->IsTileLockedWithLock(pTile))
    {
        pPlayer->SendOnTalkBubble("This area must be locked to put your item on display!", false);
        return;
    }

    if (!pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
    {
        pPlayer->SendOnTalkBubble("Only the block's owner can place items in it.", false);
        return;
    }

    if (pPlayer->GetInventory().GetCountOfItem(pItem->id) < 1)
    {
        pPlayer->SendOnTalkBubble("Your item vanished!", false);
        return;
    }

    if (pTileExtra->itemID != ITEM_ID_BLANK)
    {
        pPlayer->SendOnTalkBubble("Remove what's in there first!", false);
        return;
    }

    if (pItem->type == ITEM_TYPE_DISPLAY_BLOCK || pItem->type == ITEM_TYPE_LOCK)
    {
        pPlayer->SendOnTalkBubble("Sorry, no displaying Display Blocks or Locks.", false);
        return;
    }

    if (pItem->id == ITEM_ID_SCREEN_DOOR)
    {
        pPlayer->SendOnTalkBubble("Don't be a scammer", false);
        return;
    }

    if (pItem->HasFlag(ITEM_FLAG_UNTRADEABLE))
    {
        pPlayer->SendOnTalkBubble("You can't display untradeable items.", false);
        return;
    }

    if (pItem->type == ITEM_TYPE_PETFISH)
    {
        pPlayer->SendOnTalkBubble("If you wanna display a fish, use a fish tank!", false);
        return;
    }

    if (pItem->id == ITEM_ID_WORLD_KEY || pItem->id == ITEM_ID_GUILD_KEY || pItem->id == ITEM_ID_MAGPLANT_5000_REMOTE)
    {
        pPlayer->SendOnTalkBubble("No sir.", false);
        return;
    }

    pTileExtra->itemID = pItem->id;
    pPlayer->ModifyInventoryItem(pItem->id, -1);

    pWorld->SendPlayPositionedToAll(pPlayer, "blorb.wab");
    pWorld->ThrowItemToPositionFromPlayer(pPlayer, pTile->GetWorldPosCenter(), pItem->id, 1);
    pWorld->SendTileUpdate(pTile);
}

void DisplayBlockDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pTileX = packet.Find("tilex"_hash);
    if (!pTileX)
        return;

    auto pTileY = packet.Find("tiley"_hash);
    if (!pTileY)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    int32 tileX = 0;
    if (pTileX->GetInt(tileX) != TO_INT_SUCCESS)
        return;

    int32 tileY = 0;
    if (pTileY->GetInt(tileY) != TO_INT_SUCCESS)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if (!pTile)
        return;

    TileExtra_DisplayBlock* pTileExtra = pTile->GetExtra<TileExtra_DisplayBlock>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The display is gone!", false);
        return;
    }

    if (!pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
        return;

    if (pTileExtra->itemID == ITEM_ID_BLANK)
    {
        pPlayer->SendOnTalkBubble("There's nothing in there!", false);
        return;
    }

    if (!pPlayer->GetInventory().HaveRoomForItem(pTileExtra->itemID, 1))
    {
        pPlayer->SendOnTalkBubble("You don't have room to pick that up!", false);
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTileExtra->itemID);

    pPlayer->ModifyInventoryItem(pTileExtra->itemID, 1);
    pTileExtra->itemID = ITEM_ID_BLANK;

    pWorld->SendTileUpdate(pTile);

    if (pItem)
    {
        pPlayer->SendOnTalkBubble("You removed `5" + pItem->name, false);
    }
}
