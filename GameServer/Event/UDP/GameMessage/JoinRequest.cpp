#include "JoinRequest.h"
#include "IO/Log.h"
#include "../../../World/WorldManager.h"

void JoinRequest::Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
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

    string worldName(pName->value, pName->size);
    GetWorldManager()->PlayerJoinRequest(pPlayer, worldName);
}