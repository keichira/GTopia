#pragma once

#include "Precompiled.h"
#include "../Player/GamePlayer.h"
#include "Player/Role.h"

struct CommandInfo
{
    string usage = "";
    string desc = "";
    eRolePerm perm = ROLE_PERM_NONE;
    std::vector<uint32> aliases;
    bool disabled = false;
};

template<typename T>
class CommandBase {
public:
    static const CommandInfo& GetInfo()
    {
        return T::GetInfo();
    }

    static void Execute(GamePlayer* pPlayer, std::vector<string>& args) 
    {
        if(!CheckPerm(pPlayer))
            return;
        
        T::Execute(pPlayer, args);
    }

    static bool CheckPerm(GamePlayer* pPlayer)
    {
        if(!pPlayer)
            return false;

        Role* pRole = pPlayer->GetRole();
        if(!pRole || !pRole->HasPerm(GetInfo().perm) || GetInfo().disabled) 
        {
            pPlayer->SendOnConsoleMessage("`4Unknown command. ``Enter `$/help`` for a list of valid commands.");
            return false;
        }

        return true;
    }
};