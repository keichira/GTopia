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

    if(pPlayer->GetCharData().HasPlayMod(PLAYMOD_TYPE_GHOST)) 
    {
        pPlayer->RemovePlayMod(PLAYMOD_TYPE_GHOST);
    }
    else 
    {
        pPlayer->AddPlayMod(PLAYMOD_TYPE_GHOST);
    }
}
