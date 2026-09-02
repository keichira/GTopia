#include "TCPEvent_World.h"
#include "../../Player/PlayerManager.h"
#include "../../Server/ServerManager.h"
#include "../../World/WorldManager.h"

void TCPEvent_RenderWorld(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    ServerInfo* pServer = (ServerInfo*)pClient->data;
    if (!pServer)
        return;

    int32 subType = 0;
    if (!reader.Read<int32>(subType))
        return;

    if (subType == TCP_RENDER_REQUEST)
    {
        uint32 userID = 0;
        uint32 worldID = 0;

        if (!reader.Read<uint32>(userID) || !reader.Read<uint32>(worldID))
            return;

        WorldSession* pWorld = GetWorldManager()->GetWorldByInstanceID(worldID);
        if (!pWorld)
        {
            GetServerManager()->SendRenderResult(pServer, TCP_RESULT_FAIL, userID, worldID);
            return;
        }

        ServerInfo* pRenderServer = GetServerManager()->GetBestRenderServer();
        if (!pRenderServer)
        {
            GetServerManager()->SendRenderResult(pServer, TCP_RESULT_FAIL, userID, worldID);
            return;
        }

        GetServerManager()->SendRenderRequest(pRenderServer, userID, pWorld->databaseID);
    }
    else if (subType == TCP_RENDER_RESULT)
    {
        int32 result = 0;
        uint32 userID = 0;
        uint32 worldID = 0;

        if (!reader.Read<int32>(result) || !reader.Read<uint32>(userID) || !reader.Read<uint32>(worldID))
            return;

        PlayerSession* pPlayer = GetPlayerManager()->GetSessionByID(userID);
        if (!pPlayer)
            return;

        ServerInfo* pPlayerServer = GetServerManager()->GetServerByID(pPlayer->serverID);
        if (!pPlayerServer)
            return;

        GetServerManager()->SendRenderResult(pPlayerServer, result, userID, worldID);
    }
}

void TCPEvent_WorldInit(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    ServerInfo* pServer = (ServerInfo*)pClient->data;
    if (!pServer)
        return;

    GetWorldManager()->HandleWorldInit(reader);
}

void TCPEvent_WorldPlayerSession(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    ServerInfo* pServer = (ServerInfo*)pClient->data;
    if (!pServer)
        return;

    int32 type = 0;
    if (!reader.Read<int32>(type))
        return;

    switch (type)
    {
        case TCP_WORLD_PLAYER_JOIN:
        {
            uint32 playerID = 0;
            uint32 worldInstanceID = 0;

            if (!reader.Read<uint32>(playerID) || !reader.Read<uint32>(worldInstanceID))
                return;

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
            uint32 playerID = 0;
            uint32 worldInstanceID = 0;

            if (!reader.Read<uint32>(playerID) || !reader.Read<uint32>(worldInstanceID))
                return;

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

void TCPEvent_WorldSendPlayer(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    ServerInfo* pServer = (ServerInfo*)pClient->data;
    if (!pServer)
        return;

    GetWorldManager()->HandlePlayerJoinRequest(pServer, reader);
}