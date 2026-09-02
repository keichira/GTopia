#include "TCPEvent_Player.h"
#include "../../Player/PlayerManager.h"
#include "../../Server/ServerManager.h"

void TCPEvent_PlayerEndSession(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    ServerInfo* pServer = (ServerInfo*)pClient->data;
    if (!pServer)
        return;

    uint32 userID = 0;
    if (!reader.Read<uint32>(userID))
        return;

    PlayerSession* pPlayerSession = GetPlayerManager()->GetSessionByID(userID);
    if (pPlayerSession)
    {
        GetPlayerManager()->EndSessionByID(userID);
    }
}

void TCPEvent_PlayerCheckSession(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    ServerInfo* pServer = (ServerInfo*)pClient->data;
    if (!pServer)
        return;

    int32 netID = 0;
    uint32 userID = 0;
    uint32 token = 0;

    if (!reader.Read<int32>(netID) || !reader.Read<uint32>(userID) || !reader.Read<uint32>(token))
        return;

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