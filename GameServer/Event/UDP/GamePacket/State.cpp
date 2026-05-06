#include "State.h"

void State::Execute(GamePlayer* pPlayer, World* pWorld, GameUpdatePacket* pPacket)
{
    if(!pPlayer || !pWorld || !pPacket)
        return;

    if(pPacket->posX == 0 || pPacket->posY == 0)
        return;

    if(pPacket->HasFlag(GAME_PACKET_FLAG_FACING_LEFT)) 
    {
        pPlayer->GetCharData().SetCharFlag(CHARACTER_FLAG_FACING_LEFT);
    }
    else 
    {
        pPlayer->GetCharData().RemoveCharFlag(CHARACTER_FLAG_FACING_LEFT);
    }

    pPlayer->SetWorldPos(pPacket->posX, pPacket->posY);
    pPlayer->SendPositionToWorldPlayers();

    pPacket->netID = pPlayer->GetNetID();
    pWorld->SendGamePacketToAll(pPacket);
}