#include "TrashDialog.h"
#include "../GamePlayer.h"
#include "Utils/DialogBuilder.h"
#include "Item/ItemInfoManager.h"
#include "../../World/WorldManager.h"
#include "IO/Log.h"

void TrashDialog::Request(GamePlayer* pPlayer, uint16 itemID)
{
    if(!pPlayer) {
        return;
    }

    if(itemID == ITEM_ID_FIST || itemID == ITEM_ID_WRENCH) {
        pPlayer->PlaySFX("cant_place_tile.wav");
        pPlayer->SendOnTextOverlay("You'd be sorry to lose that");
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if(!pItem) {
        LOGGER_LOG_ERROR("Player %d tried to trash non exist item?", pPlayer->GetUserID());
        return;
    }

    uint8 invItemCount = pPlayer->GetInventory().GetCountOfItem(itemID);
    if(invItemCount == 0) {
        return;
    }

    DialogBuilder db;
    db.SetDefaultColor('o')
    ->AddLabelWithIcon("`4Trash ``" + pItem->name, pItem->id, true)
    ->AddTextBox("How many to `4destory``? (you have " + ToString(invItemCount) + ")")
    ->AddTextInput("count", "", "0", 4)
    ->EmbedData("itemID", itemID)
    ->EndDialog("trash_item", "OK", "Cancel");

    pPlayer->SendOnDialogRequest(db.Get());
}

void TrashDialog::Handle(GamePlayer* pPlayer, uint16 itemID, int16 itemCount)
{
    if(!pPlayer) {
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if(!pItem) {
        return;
    }

    if(itemCount <= 0) {
        return;
    }

    if(pItem->HasFlag(ITEM_FLAG_UNTRADEABLE)) {
        if(itemCount > pPlayer->GetInventory().GetCountOfItem(itemID)) {
            return;
        }

        DialogBuilder db;
        db.SetDefaultColor('o')
        ->AddLabelWithIcon("`4Trash ``" + ToString(itemCount) + " " + pItem->name, pItem->id, true)
        ->AddTextBox("You are recycling an `9UNTRADABLE`` item. Are you absolutely sure you want to do this? There is no way to get the item back if you click yes.")
        ->EmbedData("itemID", itemID)
        ->EmbedData("count", itemCount)
        ->EndDialog("trash_item2", "Yes, I am sure", "NO!");

        pPlayer->SendOnDialogRequest(db.Get());
        return;
    }

    pPlayer->TrashItem(itemID, itemCount);
}

void TrashDialog::HandleUntradeable(GamePlayer* pPlayer, uint16 itemID, int16 itemCount)
{
    pPlayer->TrashItem(itemID, itemCount);   
}
