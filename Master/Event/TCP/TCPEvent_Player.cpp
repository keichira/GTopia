#include "TCPEvent_Player.h"
#include "../../Player/PlayerManager.h"
#include "../../Server/ServerManager.h"

void TCPEvent_PlayerEndSession(NetClient* pClient, VariantVector& data)
{
    if (!pClient || data.size() < 2)
        return;

    ServerInfo* pServer = (ServerInfo*)pClient->data;
    if (!pServer)
        return;

    uint32 userID = data[1].GetUINT();

    PlayerSession* pPlayerSession = GetPlayerManager()->GetSessionByID(userID);
    if (pPlayerSession)
    {
        GetPlayerManager()->EndSessionByID(userID);
    }
}

void TCPEvent_PlayerCheckSession(NetClient* pClient, VariantVector& data)
{
    if (!pClient || data.size() < 4)
        return;

    ServerInfo* pServer = (ServerInfo*)pClient->data;
    if (!pServer)
        return;

    int32 netID = data[1].GetINT();
    uint32 userID = data[2].GetUINT();
    uint32 token = data[3].GetUINT();

    PlayerSession* pPlayerSession = GetPlayerManager()->GetSessionByID(userID);
    bool hasSession = true;

    if (!pPlayerSession || pPlayerSession->serverID != pServer->serverID || pPlayerSession->loginToken != token)
    {
        hasSession = false;
    }

    uint32 worldInstanceID = 0;
    if (hasSession)
    {
        worldInstanceID = pPlayerSession->worldInstanceID;
    }

    GetServerManager()->SendPlayerSessionCheck(pServer, hasSession, netID, worldInstanceID);
}
