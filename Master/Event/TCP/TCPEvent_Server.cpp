#include "TCPEvent_Server.h"
#include "../../Server/ServerManager.h"
#include "Utils/StringUtils.h"

void TCPEvent_Hello(NetClient* pClient, VariantVector& data)
{
    if (!pClient)
        return;

    ServerInfo* pServer = (ServerInfo*)pClient->data;
    if (!pServer)
        return;

    uint8 bytes[16];
    if (GetRandomBytes(&bytes, sizeof(bytes)) < 0)
    {
        pClient->status = SOCKET_CLIENT_CLOSE;
        return;
    }

    string authKey = ToHex(bytes, sizeof(bytes));
    XorCipher(authKey, SOCKET_AUTH_SECRET_KEY);

    pServer->authKey = authKey;
    GetServerManager()->SendHelloPacket(pServer, authKey);
}

void TCPEvent_Auth(NetClient* pClient, VariantVector& data)
{
    if (!pClient || data.size() < 4)
        return;

    ServerInfo* pServer = (ServerInfo*)pClient->data;
    if (!pServer)
        return;

    if (pServer->authed)
    {
        LOGGER_LOG_WARN("Server %d is authed but sent an auth packet, skipping...", pServer->serverID);
        return;
    }

    if (pServer->authKey.empty())
    {
        LOGGER_LOG_ERROR("Huh? Server %d authKey is empty, closing connection...", pServer->serverID);
        return;
    }

    string authKey = data[1].GetString();
    XorCipher(authKey, SOCKET_AUTH_SECRET_KEY);

    bool authed = true;
    if (pServer->authKey.size() != authKey.size() || pServer->authKey != authKey)
    {
        authed = false;
    }

    uint32 serverID = data[2].GetUINT();
    int32 serverType = data[3].GetINT();

    if (!authed || !GetServerManager()->AddServer(pServer, (uint16)serverID, (int8)serverType))
    {
        LOGGER_LOG_WARN("Failed to authorize server! closing connection...");
        pClient->status = SOCKET_CLIENT_CLOSE;
        GetServerManager()->SendAuthPacket(pServer, false);
        return;
    }

    LOGGER_LOG_INFO("Authed server %d, type %d", serverID, serverType);
    GetServerManager()->SendAuthPacket(pServer, true);
}

void TCPEvent_HeartBeat(NetClient* pClient, VariantVector& data)
{
    if (!pClient || data.size() < 3)
        return;

    ServerInfo* pServer = (ServerInfo*)pClient->data;
    if (!pServer)
        return;

    pServer->playerCount = data[1].GetUINT();
    pServer->worldCount = data[2].GetUINT();
    pServer->lastHeartbeatTime.Reset();
}

void TCPEvent_KillServer(NetClient* pClient, VariantVector& data)
{
    if (!pClient)
        return;

    ServerInfo* pServer = (ServerInfo*)pClient->data;
    if (!pServer)
        return;

    LOGGER_LOG_WARN("Killing server %d", pServer->serverID);
    GetServerManager()->RemoveServer(pServer->serverID);
}
