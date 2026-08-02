#include "TrashDialog.h"
#include "../../World/WorldManager.h"
#include "../GamePlayer.h"
#include "IO/Log.h"
#include "Item/ItemInfoManager.h"
#include "Utils/DialogBuilder.h"

void TrashDialog::Request(GamePlayer* pPlayer, int16 itemID)
{
    if (!pPlayer || itemID == ITEM_ID_BLANK)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
    {
        LOGGER_LOG_ERROR("Player %d tried to trash non exist item?", pPlayer->GetUserID());
        return;
    }

    if (pItem->IsUnlimited())
    {
        pPlayer->PlaySFX("cant_place_tile.wav");
        pPlayer->SendOnTextOverlay("You'd be sorry to lose that");
        return;
    }

    uint8 invItemCount = pPlayer->GetInventory().GetCountOfItem(pItem->id);
    if (invItemCount == 0)
        return;

    DialogBuilder db;
    db.SetDefaultColor('o');
    if (pPlayer->HasFlag(PLAYER_FLAG_SUPPORTER) || pPlayer->HasFlag(PLAYER_FLAG_SUPER_SUPPORTER))
    {
        db.AddLabelWithIcon("`4Recycle`` " + pItem->name, pItem->id, true);
    }
    else
    {
        db.AddLabelWithIcon("`4Trash`` " + pItem->name, pItem->id, true);
    }

    db.AddTextBox("How many to `4destory``? (you have " + ToString(invItemCount) + ")")
        .AddTextInput("count", "", "0", 4)
        .EmbedData("itemID", ToItemClientID(pItem->id))
        .EndDialog("trash_item", "OK", "Cancel");

    pPlayer->SendOnDialogRequest(db.Get());
}

void TrashDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pItemID = packet.Find("itemID"_hash);
    auto pCount = packet.Find("count"_hash);

    if (!pItemID || !pCount)
        return;

    uint32 itemID = 0;
    if (pItemID->GetUInt(itemID) != TO_INT_SUCCESS)
        return;

    int32 count = 0;
    if (pCount->GetInt(count) != TO_INT_SUCCESS)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
        return;

    if (count <= 0)
        return;

    if (pItem->HasFlag(ITEM_FLAG_UNTRADEABLE))
    {
        if (count > pPlayer->GetInventory().GetCountOfItem(pItem->id))
            return;

        DialogBuilder db;
        db.SetDefaultColor('o')
            .AddLabelWithIcon("`4Trash ``" + ToString(count) + " " + pItem->name, pItem->id, true)
            .AddTextBox("You are recycling an `9UNTRADABLE`` item. Are you absolutely sure you want to do this? There "
                        "is no way to get the item back if you click yes.")
            .EmbedData("itemID", ToItemClientID(pItem->id))
            .EmbedData("count", count)
            .EndDialog("trash_item2", "Yes, I am sure", "NO!");

        pPlayer->SendOnDialogRequest(db.Get());
        return;
    }

    pPlayer->TrashItem(pItem->id, count);
}

void TrashDialog::HandleUntradeable(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pItemID = packet.Find("itemID"_hash);
    auto pCount = packet.Find("count"_hash);

    if (!pItemID || !pCount)
        return;

    uint32 itemID = 0;
    if (pItemID->GetUInt(itemID) != TO_INT_SUCCESS)
        return;

    int32 count = 0;
    if (pCount->GetInt(count) != TO_INT_SUCCESS)
        return;

    if (count <= 0)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
        return;

    if (!pItem->HasFlag(ITEM_FLAG_UNTRADEABLE))
        return;

    pPlayer->TrashItem(itemID, count);
}
