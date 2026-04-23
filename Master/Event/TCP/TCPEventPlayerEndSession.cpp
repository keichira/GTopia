#include "TCPEventPlayerEndSession.h"
#include "../../Player/PlayerManager.h"
#include "../../Server/ServerManager.h"

void TCPPlayerEndSessionEventData::FromVariant(VariantVector& varVec)
{
    if(varVec.size() < 2) {
        return;
    }

    userID = varVec[1].GetUINT();
}

void TCPEventPlayerEndSession::Execute(NetClient* pClient, VariantVector& data)
{
    TCPPlayerEndSessionEventData eventData;
    eventData.FromVariant(data);

    PlayerSession* pPlayer = GetPlayerManager()->GetSessionByID(eventData.userID);
    if(pPlayer) {
        GetPlayerManager()->EndSessionByID(pPlayer->userID);
    }
}