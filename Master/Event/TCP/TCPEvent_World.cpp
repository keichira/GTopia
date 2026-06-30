#include "TCPEvent_World.h"
#include "../../Player/PlayerManager.h"
#include "../../Server/ServerManager.h"
#include "../../World/WorldManager.h"

void TCPEvent_RenderWorld(NetClient* pClient, VariantVector& data)
{
    if (!pClient || data.size() < 4)
        return;

    ServerInfo* pServer = (ServerInfo*)pClient->data;
    if (!pServer)
        return;

    int32 subType = data[1].GetINT();
    int32 result;
    uint32 userID;
    uint32 worldID;

    WorldSession* pWorld = nullptr;
    if (subType == TCP_RENDER_REQUEST)
    {
        userID = data[2].GetUINT();
        worldID = data[3].GetUINT();

        pWorld = GetWorldManager()->GetWorldByInstanceID(worldID);

        if (!pWorld)
        {
            GetServerManager()->SendRenderResult(pServer, TCP_RESULT_FAIL, userID, worldID);
            return;
        }
    }
    else
    {
        result = data[2].GetINT();
        userID = data[3].GetUINT();
        worldID = data[4].GetUINT();
    }

    switch (subType)
    {
        case TCP_RENDER_REQUEST:
        {
            ServerInfo* pRenderServer = GetServerManager()->GetBestRenderServer();
            if (!pRenderServer)
            {
                GetServerManager()->SendRenderResult(pServer, TCP_RESULT_FAIL, userID, worldID);
                return;
            }

            GetServerManager()->SendRenderRequest(pRenderServer, userID, pWorld->databaseID);
            break;
        }

        case TCP_RENDER_RESULT:
        {
            PlayerSession* pPlayer = GetPlayerManager()->GetSessionByID(userID);
            if (!pPlayer)
                return;

            ServerInfo* pPlayerServer = GetServerManager()->GetServerByID(pPlayer->serverID);
            if (!pPlayerServer)
                return;

            GetServerManager()->SendRenderResult(pPlayerServer, result, userID, worldID);
            break;
        }
    }
}

void TCPEvent_WorldInit(NetClient* pClient, VariantVector& data)
{
    if (!pClient)
        return;

    ServerInfo* pServer = (ServerInfo*)pClient->data;
    if (!pServer)
        return;

    GetWorldManager()->HandleWorldInit(std::move(data));
}

void TCPEvent_WorldPlayerSession(NetClient* pClient, VariantVector& data)
{
    if (!pClient || data.size() < 2)
        return;

    ServerInfo* pServer = (ServerInfo*)pClient->data;
    if (!pServer)
        return;

    int32 type = data[1].GetINT();

    switch (type)
    {
        case TCP_WORLD_PLAYER_JOIN:
        {
            uint32 playerID = data[1].GetUINT();
            uint32 worldInstanceID = data[1].GetUINT();

            WorldSession* pWorld = GetWorldManager()->GetWorldByInstanceID(worldInstanceID);
            if (pWorld)
            {
                pWorld->playerCount++;
            }

            PlayerSession* pPlayer = GetPlayerManager()->GetSessionByID(playerID);
            if (pPlayer)
            {
                pPlayer->worldInstanceID = worldInstanceID;
            }

            break;
        }

        case TCP_WORLD_PLAYER_LEAVE:
        {
            uint32 playerID = data[1].GetUINT();
            uint32 worldInstanceID = data[1].GetUINT();

            WorldSession* pWorld = GetWorldManager()->GetWorldByInstanceID(worldInstanceID);
            if (pWorld)
            {
                if (pWorld->playerCount != 0)
                {
                    pWorld->playerCount--;
                }
            }

            PlayerSession* pPlayer = GetPlayerManager()->GetSessionByID(playerID);
            if (pPlayer)
            {
                pPlayer->worldInstanceID = 0;
            }

            break;
        }
    }
}

void TCPEvent_WorldSendPlayer(NetClient* pClient, VariantVector& data)
{
    if (!pClient || data.size() < 3)
        return;

    ServerInfo* pServer = (ServerInfo*)pClient->data;
    if (!pServer)
        return;

    GetWorldManager()->HandlePlayerJoinRequest(pServer, std::move(data));
}
