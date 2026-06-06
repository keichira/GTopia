#include "Utils/StringUtils.h"
#include "Emotes.h"
#include "../World/WorldManager.h"

const CommandInfo& Emotes::GetInfo()
{
    static CommandInfo info =
    {
        "",
        "",
        0,
        {
            "dance"_hash,
            "dance2"_hash,
            "facepalm"_hash,
            "mad"_hash,
            "shrug"_hash,
            "foldarms"_hash,
            "stubborn"_hash,
            "fold"_hash,
            "rolleyes"_hash,
            "eyeroll"_hash,
            "sad"_hash,
            "fa"_hash,
            "idk"_hash,
            "no"_hash,
            "omg"_hash,
            "yes"_hash,
            "sleep"_hash,
            "cheer"_hash,
            "troll"_hash,
            "love"_hash,
            "kiss"_hash,
            "wave"_hash,
            "fp"_hash
        }
    };

    return info;
}

void Emotes::Execute(GamePlayer* pPlayer, std::vector<string>& args)
{
    if(!pPlayer || args.empty() || !CheckPerm(pPlayer))
        return;

    if(pPlayer->GetLastActionTime().GetElapsedTime() <= 200)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    pPlayer->GetLastActionTime().Reset();
    pWorld->SendOnActionToAll(pPlayer, args[0]);
    pWorld->CheckOuijaBoardCommand(pPlayer, args[0]);
}
