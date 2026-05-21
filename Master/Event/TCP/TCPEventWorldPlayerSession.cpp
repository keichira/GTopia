#include "TCPEventWorldPlayerSession.h"
#include "../../World/WorldManager.h"
#include "../../Player/PlayerManager.h"
#include "../../Server/ServerManager.h"

void TCPEventWorldPlayerSession::Execute(NetClient* pClient, VariantVector& data)
{
    if(!pClient)
        return;

    if(data.size() < 2)
        return;

    int32 type = data[1].GetINT();

    switch(type)
    {
        case TCP_WORLD_PLAYER_JOIN:
        {
            uint32 playerID = data[1].GetUINT();
            uint32 worldInstanceID = data[1].GetUINT();

            WorldSession* pWorld = GetWorldManager()->GetWorldByInstanceID(worldInstanceID);
            if(pWorld)
            {
                pWorld->playerCount++;
            }

            PlayerSession* pPlayer = GetPlayerManager()->GetSessionByID(playerID);
            if(pPlayer)
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
            if(pWorld)
            {
                if(pWorld->playerCount != 0)
                {
                    pWorld->playerCount--;
                }
            }

            PlayerSession* pPlayer = GetPlayerManager()->GetSessionByID(playerID);
            if(pPlayer)
            {
                pPlayer->worldInstanceID = 0;
            }

            break;
        }
    }
}