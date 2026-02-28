#include "JoinRequest.h"
#include "IO/Log.h"
#include "../../../World/WorldManager.h"

void JoinRequest::Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    auto pName = packet.Find(CompileTimeHashString("name"));
    if(!pName) {
        pPlayer->SendOnFailedToEnterWorld();
        pPlayer->SendOnConsoleMessage("Unable to enter world by unknown reason");
        LOGGER_LOG_WARN("Player tried to join a world but name field missing??");
        return;
    }

    if(pName->size < 1 || pName->size > 24) {
        pPlayer->SendOnFailedToEnterWorld();
        pPlayer->SendOnConsoleMessage("Unable to enter, world name length must between 1 and 24");
        return;
    }

    string worldName(pName->value, pName->size);
    GetWorldManager()->PlayerJoinRequest(pPlayer, ToUpper(worldName));
}