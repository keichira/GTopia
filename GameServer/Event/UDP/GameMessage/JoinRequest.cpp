#include "JoinRequest.h"
#include "IO/Log.h"
#include "../../../World/WorldManager.h"

void JoinRequest::Execute(GamePlayer* pPlayer, ParsedTextPacket<40>& packet)
{
    if(pPlayer->GetLastJoinRequestTime().GetElapsedTime() <= 800)
        return;
        
    pPlayer->GetLastJoinRequestTime().Reset();

    auto pName = packet.Find("name"_hash);
    if(!pName) 
    {
        pPlayer->SendOnFailedToEnterWorld();
        return;
    }

    GetWorldManager()->PlayerJoinRequest(pPlayer, pName->GetString());
}