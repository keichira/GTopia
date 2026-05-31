#include "Utils/StringUtils.h"
#include "AgeWorld.h"
#include "../World/WorldManager.h"

const CommandInfo& AgeWorld::GetInfo()
{
    static CommandInfo info =
    {
        "/ageworld <ageMin>",
        "Age current map",
        ROLE_PERM_MSTATE,
        {
            "ageworld"_hash
        }
    };

    return info;
}

void AgeWorld::Execute(GamePlayer* pPlayer, std::vector<string>& args)
{
    if(!pPlayer || args.empty() || !CheckPerm(pPlayer))
        return;

    if(args.size() < 2)
    {
        pPlayer->SendOnConsoleMessage("`oUsage: " + GetInfo().usage);
        return;
    }

    uint32 ageMin = 0;
    if(ToUInt(args[1], ageMin) != TO_INT_SUCCESS)
    {
        pPlayer->SendOnConsoleMessage("`oAgeMin must be positive number.");
        return;
    }

    if(ageMin > 100000)
    {
        pPlayer->SendOnConsoleMessage("`oUmm, the target age seems like so old... please age it a bit lesser");
        return;
    }

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    pWorld->GetTileManager()->AgeTiles(ageMin * 60 * 1000);
    pPlayer->SendOnConsoleMessage("World \"`#" + pWorld->GetWorlName() + "``\" aged " + ToString(ageMin) + " minutes.");
    pWorld->ReconnectPlayers();
}
