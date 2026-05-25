#include "Utils/StringUtils.h"
#include "Ghost.h"

const CommandInfo& Ghost::GetInfo()
{
    static CommandInfo info =
    {
        "/ghost",
        "Walk throught blocks",
        ROLE_PERM_COMMAND_GHOST,
        {
            CompileTimeHashString("ghost")
        }
    };

    return info;
}

void Ghost::Execute(GamePlayer* pPlayer, std::vector<string>& args)
{
    if(!pPlayer || args.empty() || !CheckPerm(pPlayer))
        return;

    PlayerPlayModController& modController = pPlayer->GetModController();

    if(modController.HasPlayMod(PLAYMOD_TYPE_GHOST)) 
    {
        modController.RemovePlayMod(PLAYMOD_TYPE_GHOST);
    }
    else 
    {
        modController.AddPlayMod(PLAYMOD_TYPE_GHOST);
    }
}
