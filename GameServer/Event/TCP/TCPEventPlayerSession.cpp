#include "TCPEventPlayerSession.h"
#include "../../Player/PlayerManager.h"
#include "../../Player/GamePlayer.h"
#include "IO/Log.h"

void TCPPlayerSessionEventData::FromVariant(const VariantVector& varVec)
{
    if(varVec.size() < 2) {
        return;
    }

    playerNetID = varVec[1].GetINT();
}

void TCPEventPlayerSession::Execute(NetClient* pClient, VariantVector& data)
{
    TCPPlayerSessionEventData eventData;
    eventData.FromVariant(data);

    if(eventData.playerNetID <= 0) {
        return;
    }

    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(eventData.playerNetID);
    if(!pPlayer) {
        LOGGER_LOG_WARN("Received player session packet but player not found?");
        return;
    }

    pPlayer->HandleCheckSession(std::move(data));
}
