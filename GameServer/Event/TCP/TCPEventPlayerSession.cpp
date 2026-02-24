#include "TCPEventPlayerSession.h"
#include "../../Server/GameServer.h"
#include "../../Player/GamePlayer.h"
#include "Packet/GamePacket.h"
#include "IO/Log.h"

void TCPEventPlayerSession::Execute(NetClient* pClient, VariantVector& data)
{
    Player* pPlayer = GetGameServer()->GetPlayerByNetID(data[1].GetINT());
    if(!pPlayer) {
        LOGGER_LOG_WARN("Received player session packet but player not found?");
        return;
    }

    pPlayer->OnHandleTCP(std::move(data));
}