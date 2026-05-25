#include "State.h"
#include "Math/Math.h"

void State::Execute(GamePlayer* pPlayer, World* pWorld, GameUpdatePacket* pPacket)
{
    if(!pPlayer || !pWorld || !pPacket)
        return;

    if(IsNan(pPacket->field_8.x) || IsNan(pPacket->field_8.y))
        return;

    if(IsNan(pPacket->field_9.x) || IsNan(pPacket->field_9.y))
        return;

    if(pPacket->field_8.x == 0 || pPacket->field_8.y == 0)
        return;

    if(pPacket->HasFlag(GAME_PACKET_FLAG_FACING_LEFT)) 
    {
        pPlayer->GetCharData().SetCharFlag(CHARACTER_FLAG_FACING_LEFT);
    }
    else 
    {
        pPlayer->GetCharData().RemoveCharFlag(CHARACTER_FLAG_FACING_LEFT);
    }

    pPlayer->SetWorldPos(pPacket->field_8.x, pPacket->field_8.y);
    pPlayer->SendPositionToWorldPlayers();

    pPacket->field_4 = pPlayer->GetNetID();
    pWorld->SendGamePacketToAll(pPacket);
}