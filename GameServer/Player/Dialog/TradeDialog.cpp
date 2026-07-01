#include "TradeDialog.h"
#include "../../World/WorldManager.h"
#include "../GamePlayer.h"
#include "Item/ItemInfoManager.h"
#include "Utils/DialogBuilder.h"

void TradeDialog::Request(GamePlayer* pPlayer, InventoryItemInfo* pInvItem)
{
    if (!pPlayer || !pInvItem)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pInvItem->id);
    if (!pItem)
        return;

    if (pPlayer->GetCurrentWorld() == 0)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    DialogBuilder db;
    db.SetDefaultColor('o')
        ->AddLabelWithIcon("`2Trade`w" + pItem->name, pItem->id, true)
        ->AddTextBox("`2Trade how many?``")
        ->AddTextInput("count", "", "", 5)
        ->EmbedData("itemID", ToItemClientID(pItem->id))
        ->EndDialog("trade_item", "OK", "Cancel");

    pPlayer->SendOnDialogRequest(db.Get());
}

void TradeDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pItemID = packet.Find("itemID"_hash);
    if (!pItemID)
        return;

    auto pCount = packet.Find("count"_hash);
    if (!pCount)
        return;

    if (pPlayer->GetCurrentWorld() == 0)
        return;

    int32 itemID = 0;
    if (pItemID->GetInt(itemID) != TO_INT_SUCCESS)
        return;

    int32 count = 0;
    if (pCount->GetInt(count) != TO_INT_SUCCESS)
        return;

    if (count <= 0)
        return;

    pPlayer->GetTradeManager().OnTradeItem(itemID, count, false);
}
