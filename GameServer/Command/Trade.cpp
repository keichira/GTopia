#include "Trade.h"
#include "../World/WorldManager.h"
#include "Utils/StringUtils.h"

const CommandInfo& Trade::GetInfo()
{
    static CommandInfo info = {"/trade <playerName>", "Start trade with someone", 0, {"trade"_hash}};

    return info;
}

void Trade::Execute(GamePlayer* pPlayer, std::vector<string>& args)
{
    if (!pPlayer || args.empty() || !CheckPerm(pPlayer))
        return;

    if (args.size() < 2)
    {
        pPlayer->SendOnConsoleMessage("`oUsage: " + GetInfo().usage);
        return;
    }

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    if (pWorld->GetPlayerCount() <= 1)
    {
        pPlayer->SendOnConsoleMessage("There are no people to trade with.");
        return;
    }

    string error;
    GamePlayer* pTarget = pWorld->GetPlayerByNameStartsWith(args[1], error);
    if (!pTarget)
    {
        pPlayer->SendOnConsoleMessage(error);
        return;
    }

    if (pTarget->GetTradeManager().IsTrading() && pTarget->GetTradeManager().GetPartner() != pPlayer)
    {
        pPlayer->SendOnTalkBubble("That person is busy", false);
        return;
    }

    pPlayer->SendOnStartTrade(pTarget->GetDisplayName(true), pTarget->GetNetID());
}
