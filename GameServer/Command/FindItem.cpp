#include "FindItem.h"
#include "../Player/Dialog/ItemFinderDialog.h"
#include "../World/WorldManager.h"
#include "Utils/StringUtils.h"

const CommandInfo& FindItem::GetInfo()
{
    static CommandInfo info = {
        "/finditem <item_name>", "Find item from database", "command.finditem"_hash, {"finditem"_hash}};

    return info;
}

void FindItem::Execute(GamePlayer* pPlayer, std::vector<string>& args)
{
    if (!pPlayer || args.empty() || !CheckPerm(pPlayer))
        return;

    if (args.size() < 1)
    {
        pPlayer->SendOnConsoleMessage("`oUsage: " + GetInfo().usage);
        return;
    }

    string itemName = JoinString(args, " ", 1);
    StripWhiteSpace(itemName);

    if (itemName.size() < 3)
    {
        pPlayer->SendOnConsoleMessage("`4Error:`` Item name must be at least `w3`` characters long.");
        return;
    }

    auto items = GetItemInfoManager()->GetSortedItemIdsByName(itemName, true);
    if (items.empty())
    {
        pPlayer->SendOnConsoleMessage("`4Error:`` No items matched your search criteria.");
        return;
    }

    if (items.size() > 5000)
    {
        pPlayer->SendOnConsoleMessage("`4Error:`` Too many results found. Please provide a more specific item name.");
        return;
    }

    pPlayer->OpenPaginatedDialog(std::make_unique<ItemFinderDialog>(std::move(items)));
}
