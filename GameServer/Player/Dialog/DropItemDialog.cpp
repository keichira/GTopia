#include "DropItemDialog.h"
#include "../GamePlayer.h"
#include "Utils/DialogBuilder.h"
#include "Item/ItemInfoManager.h"
#include "../../World/WorldManager.h"
#include "IO/Log.h"

void DropItemDialog::Request(GamePlayer* pPlayer, InventoryItemInfo* pInvItem)
{
    if(!pPlayer || !pInvItem)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pInvItem->id);
    if(!pItem) 
        return;

    if(pPlayer->GetCurrentWorld() == 0)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;
        
    DialogBuilder db;
    db.SetDefaultColor('o')
    ->AddLabelWithIcon("`wDrop " + pItem->name + "``", pItem->id, true)
    ->AddTextBox("How many to drop?")
    ->AddTextInput("count", "", ToString(pInvItem->count), 5)
    ->EmbedData("itemID", pItem->id);

    bool showWarning = false;

    /*TileInfo* pLockTile = pWorld->GetTileManager()->GetKeyTile(KEY_TILE_WORLD_LOCK);
    if(!pLockTile)
    {
        showWarning = true;
    }

    TileExtra_Lock* pLockExtra = pLockTile->GetExtra<TileExtra_Lock>();
    if(!pLockExtra)
    {
        showWarning = true;
    }
    else {
        if(!pLockExtra->HasAccess(pPlayer->GetUserID()))
        {
            showWarning = true;
        }
    }

    if(showWarning)
    {
        switch(RandomRangeInt(0, 4))
        {
            case 0:
                db.AddTextBox("`4Warning:`` Once you drop an item, it is no longer yours...");
                break;
            case 1:
                db.AddTextBox("`4Warning:`` Dropped items cannot be restored...");
                break;
            case 2:
                db.AddTextBox("`4Warning:`` Scammers may ask you to drop items...");
                break;
            case 3:
                db.AddTextBox("`4Warning:`` Players asking you to drop items may be scamming you.");
                break;
            case 4:
                db.AddTextBox("If trading, use the trade system instead of dropping items!");
                break;
        }
    }*/

    db.EndDialog("drop_item", "OK", "Cancel");
    pPlayer->SendOnDialogRequest(db.Get());
}

void DropItemDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    if(!pPlayer || pPlayer->GetCurrentWorld() == 0)
        return;

    auto pItemID = packet.Find("itemID"_hash);
    auto pCount = packet.Find("count"_hash);

    if(!pItemID || !pCount)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    uint32 itemID = 0;
    if(ToUInt(string(pItemID->value, pItemID->size), itemID) != TO_INT_SUCCESS)
        return;

    int32 count = 0;
    if(ToInt(string(pCount->value, pCount->size), count) != TO_INT_SUCCESS)
        return;


    if(count == 0)
        return;

    if(count < 0)
    {
        pPlayer->SendOnTalkBubble("Nice try. You remind me of myself at that age.", true);
        return;
    }

    pPlayer->DropItem(itemID, count, false);
}
