#include "Utils/StringUtils.h"
#include "GiveItem.h"
#include "../Player/PlayerManager.h"
#include "Item/ItemInfoManager.h"

const CommandInfo& GiveItem::GetInfo()
{
    static CommandInfo info =
    {
        "/giveitem <userID> <amount> <item name>",
        "Give item to player",
        ROLE_PERM_MSTATE, // change this
        {
            CompileTimeHashString("giveitem")
        }
    };

    return info;
}

void GiveItem::Execute(GamePlayer* pPlayer, std::vector<string>& args)
{
    if(!pPlayer || args.empty() || !CheckPerm(pPlayer)) {
        return;
    }

    if(args.size() < 4) {
        pPlayer->SendOnConsoleMessage("`oUsage: " + GetInfo().usage);
        return;
    }

    uint32 userID = 0;
    if(ToUInt(args[1], userID) != TO_INT_SUCCESS) {
        pPlayer->SendOnConsoleMessage("`oUserID must be number!");
        return;
    }

    uint32 amount = 0;
    if(ToUInt(args[2], amount) != TO_INT_SUCCESS) {
        pPlayer->SendOnConsoleMessage("`oItem amount must be number!");
        return;
    }

    string itemName = JoinString(args, " ", 3);
    ItemInfo* pItem = GetItemInfoManager()->GetItemByName(itemName);
    if(!pItem) {
        pPlayer->SendOnConsoleMessage("`oFailed to find given item " + itemName);
        return;
    }

    GamePlayer* pTarget = GetPlayerManager()->GetPlayerByUserID(userID);
    if(!pTarget) { // todo search on sub-servers
        pPlayer->SendOnConsoleMessage("`oFailed to find user with given id");
        return;
    }

    uint8 givenCount = pTarget->GetInventory().AddItem(pItem->id, amount, pTarget);
    if(givenCount == 0) {
        /**
         * handle
         */
    }

    pPlayer->SendOnConsoleMessage("`oGiven " + ToString(givenCount) + "x " + pItem->name + " to " + pTarget->GetRawName() + " (ID: " + ToString(pTarget->GetUserID()) + ")");
    pTarget->SendOnConsoleMessage("`oGiven: " + ToString(givenCount) + "x " + pItem->name);
}
