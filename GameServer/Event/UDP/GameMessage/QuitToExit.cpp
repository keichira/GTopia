#include "QuitToExit.h"
#include "../../../World/WorldManager.h"

void QuitToExit::Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    if(!pPlayer)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    pWorld->PlayerLeaveWorld(pPlayer);
}