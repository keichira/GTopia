#include "TCPEventPlayerSession.h"
#include "../../Player/PlayerManager.h"
#include "../../Server/ServerManager.h"

void TCPPlayerSessionEventData::FromVariant(VariantVector& varVec)
{
    if(varVec.size() < 4) {
        return;
    }

    netID = varVec[1].GetINT();
    userID = varVec[2].GetUINT();
    token = varVec[3].GetUINT();
}

void TCPEventPlayerSession::Execute(NetClient* pClient, VariantVector& data)
{
    if(!pClient) {
        return;
    }

    ServerInfo* pServer = (ServerInfo*)pClient->data;
    if(!pServer) {
        return;
    }

    TCPPlayerSessionEventData eventData;
    eventData.FromVariant(data);

    PlayerSession* pPlayer = GetPlayerManager()->GetSessionByID(eventData.userID);
    bool hasSession = true;

    if(
        !pPlayer ||
        pPlayer->serverID != pServer->serverID ||
        pPlayer->loginToken != eventData.token
    ) {
        hasSession = false;
    }

    uint32 worldInstance = 0;
    if(hasSession) {
        worldInstance = pPlayer->worldInstanceID;
    }

    GetServerManager()->SendPlayerSessionCheck(pServer, hasSession, eventData.netID, worldInstance);
}