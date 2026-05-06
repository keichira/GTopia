#include "Utils/StringUtils.h"
#include "TogglePlayMod.h"
#include "../Server/MasterBroadway.h"

const CommandInfo& TogglePlayMod::GetInfo()
{
    static CommandInfo info =
    {
        "/toggleplaymod",
        "Toggle playmods",
        ROLE_PERM_SMSTATE,
        {
            CompileTimeHashString("toggleplaymod")
        }
    };

    return info;
}

void TogglePlayMod::Execute(GamePlayer* pPlayer, std::vector<string>& args)
{
    if(!pPlayer || args.empty() || !CheckPerm(pPlayer))
        return;

    if(args.size() < 2) 
    {
        pPlayer->SendOnConsoleMessage("`oUsage: " + GetInfo().usage);
        return;
    }

    uint32 playModID = 0;
    if(ToUInt(args[1], playModID) != TO_INT_SUCCESS) 
    {
        pPlayer->SendOnConsoleMessage("`PlayModID must be number!");
        return;
    }

    if(pPlayer->GetCharData().HasPlayMod((ePlayModType)playModID)) 
    {
        pPlayer->RemovePlayMod((ePlayModType)playModID);
    }
    else 
    {
        pPlayer->AddPlayMod((ePlayModType)playModID);
    }
}
