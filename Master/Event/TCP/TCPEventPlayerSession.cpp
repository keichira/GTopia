#include "TCPEventPlayerSession.h"
#include "../../Player/PlayerManager.h"
#include "../../Server/ServerManager.h"

void TCPPlayerSessionEventData::FromVariant(VariantVector& varVec)
{
    if(varVec.size() < 5) {
        return;
    }

    netID = varVec[1].GetINT();
    userID = varVec[2].GetUINT();
    token = varVec[3].GetUINT();
    serverID = varVec[4].GetUINT();
}

void TCPEventPlayerSession::Execute(NetClient* pClient, VariantVector& data)
{
    TCPPlayerSessionEventData eventData;
    eventData.FromVariant(data);

    PlayerSession* pPlayer = GetPlayerManager()->GetSessionByID(eventData.userID);
    bool hasSession = true;

    if(
        !pPlayer ||
        pPlayer->serverID != eventData.serverID ||
        pPlayer->loginToken != eventData.token
    ) {
        hasSession = false;
    }

    GetServerManager()->SendPlayerSessionCheck(hasSession, eventData.netID, pClient->connectionID);
}