#include "ItemFinderDialog.h"
#include "../GamePlayer.h"
#include "Item/ItemInfoManager.h"
#include "Math/Math.h"
#include "Utils/DialogBuilder.h"
#include "Utils/StringUtils.h"

/**
 * i need help with dialog im so bad at it lol
 */

void ItemFinderDialog::Render(GamePlayer* pPlayer)
{
    if (!pPlayer)
        return;

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon("Item Finder", ITEM_ID_GROWSCAN_9000, true)
        .AddSmallText("`2" + ToString(GetTotalCount()) + "`` items found. Listing `w" + ToString(m_pageSize) +
                      "`` items per page (Page `w" + ToString(m_page + 1) + "`5/`w" + ToString(GetMaxPageCount()) +
                      "``)")
        .AddSpacer();

    uint32 start = m_page * m_pageSize;
    uint32 end = Min(start + m_pageSize, GetTotalCount());

    for (uint32 i = start; i < end; ++i)
    {
        db.AddButtonWithIcon(ToString(i - start), "", m_fileteredItemIds[i]);
    }

    db.AddSpacer(true);

    if (m_page > 0)
        db.AddButton("prev", "`5< Previous");
    if (end < GetTotalCount())
        db.AddButton("next", "`5Next >");

    db.EndDialog("item_finder", "", "Close");
    pPlayer->SendOnDialogRequest(db.Get(), -1, true);
}

void ItemFinderDialog::OnSelectElement(GamePlayer* pPlayer, uint32 absoluteIndex)
{
    if (!pPlayer)
        return;

    Role* pRole = pPlayer->GetRole();
    if (!pRole || !pRole->HasPerm("command.finditem"_hash))
        return;

    int32 itemID = m_fileteredItemIds[absoluteIndex];
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
    {
        pPlayer->SendOnConsoleMessage("`4HUH?!`` Looks like the item lost in the universe.");
        return;
    }

    int32 itemCount = Max(0, pItem->maxCanHold - pPlayer->GetInventory().GetCountOfItem(pItem->id));
    if (itemCount == 0 || !pPlayer->GetInventory().HaveRoomForItem(itemID, itemCount))
    {
        pPlayer->SendOnConsoleMessage("`4Error``: You don't have room for that item!");
        return;
    }

    pPlayer->ModifyInventoryItem(itemID, itemCount);
    pPlayer->SendOnConsoleMessage("`oGiven: " + ToString(itemCount) + " " + pItem->name);
    pPlayer->ClosePaginatedDialog();
}
