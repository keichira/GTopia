#include "QuitToExit.h"
#include "../../../World/WorldManager.h"

void QuitToExit::Execute(GamePlayer* pPlayer, ParsedTextPacket<40>& packet)
{
    if(!pPlayer)
        return;

    if(pPlayer->GetCurrentWorld() == 0)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    pWorld->PlayerLeaveWorld(pPlayer, true);
}