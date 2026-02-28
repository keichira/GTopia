#include "TCPEventPlayerSession.h"
#include "../../Server/GameServer.h"
#include "Packet/GamePacket.h"

void TCPEventPlayerSession::Execute(NetClient* pClient, VariantVector& data)
{
    PlayerSession* pPlayer = GetGameServer()->GetPlayerSessionByUserID(data[2].GetUINT());

    VariantVector packet(3);
    packet[0] = TCP_PACKET_PLAYER_CHECK_SESSION;
    packet[1] = data[1].GetINT();

    if(
        !pPlayer ||
        pPlayer->serverID != data[4].GetUINT() ||
        pPlayer->loginToken != data[3].GetUINT()
    ) {
        packet[2] = false;
    }
    else {
        packet[2] = true;
    }

    pClient->Send(packet);
}