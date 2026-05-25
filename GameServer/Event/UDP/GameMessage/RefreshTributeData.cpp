#include "RefreshTributeData.h"
#include "Packet/NetPacket.h"
#include "Player/PlayerTribute.h"
#include "IO/Log.h"

void RefreshTributeData::Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    PlayerTributeClientData* clientData = GetPlayerTributeManager()->GetClientData(pPlayer->GetLoginDetail().protocol);
    if(!clientData->pData) 
    {
        LOGGER_LOG_WARN("Not sending player tribute data because its NULL");
        return;
    }

    GameUpdatePacket gamePacket;
    gamePacket.type = NET_GAME_PACKET_SEND_PLAYER_TRIBUTE_DATA;
    gamePacket.field_4 = -1;
    gamePacket.flags |= GAME_PACKET_FLAG_EXTENDED_DATA;
    gamePacket.extraDataSize = clientData->size;

    SendENetPacketRaw(NET_MESSAGE_GAME_PACKET, &gamePacket, sizeof(GameUpdatePacket), clientData->pData, pPlayer->GetPeer());
}