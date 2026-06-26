#include "JoinRequest.h"
#include "IO/Log.h"
#include "../../../World/WorldManager.h"

void JoinRequest::Execute(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if(pPlayer->GetLastJoinRequestTime().GetElapsedTime() <= 800)
        return;
        
    pPlayer->GetLastJoinRequestTime().Reset();

    auto pName = packet.Find("name"_hash);
    if(!pName || pName->valueSize == 0) 
    {
        pPlayer->SendOnFailedToEnterWorld();
        return;
    }

    /**
     * cant support name|doorid since text packet parsing doesnt allows 2 values
     */

    pPlayer->SetTargetJoinWorld(pName->GetString());
}