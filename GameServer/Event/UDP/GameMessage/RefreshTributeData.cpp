#include "RefreshTributeData.h"
#include "Packet/NetPacket.h"
#include "Player/PlayerTribute.h"
#include "IO/Log.h"

void RefreshTributeData::Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    PlayerTributeClientData clientData = GetPlayerTributeManager()->GetClientData();
    if(!clientData.pData) {
        LOGGER_LOG_WARN("Not sending player tribute data because its NULL");
        return;
    }

    GameUpdatePacket gamePacket;
    gamePacket.type = NET_GAME_PACKET_SEND_PLAYERRIBUTE_DATA;
    gamePacket.netID = -1;
    gamePacket.flags |= NET_GAME_PACKET_FLAGS_EXTENDED;

    gamePacket.extraDataSize = clientData.size;

    SendENetPacketRaw(NET_MESSAGE_GAME_PACKET, &packet, sizeof(GameUpdatePacket), clientData.pData, pPlayer->GetPeer());
}