#include "TCPEventRenderWorld.h"
#include "../../Server/ServerManager.h"
#include "../../Server/GameServer.h"
#include "../../World/WorldManager.h"

void TCPEventRenderWorldData::FromVariant(VariantVector& varVec, bool forResult)
{
    if(varVec.size() < 2) {
        return;
    }

    if(forResult) {
        result = varVec[2].GetINT();
        userID = varVec[3].GetUINT();
        worldID = varVec[4].GetUINT();
    }
    else {
        userID = varVec[2].GetUINT();
        worldID = varVec[3].GetUINT();
    }
}

void TCPEventRenderWorld::Execute(NetClient* pClient, VariantVector& data)
{
    if(data.size() < 2) {
        return;
    }

    TCPEventRenderWorldData eventData;
    int32 subType = data[1].GetINT();

    NetServerInfo* pNetServer = (NetServerInfo*)pClient->data;
    if(!pNetServer) {
        return;
    }

    string worldName;
    WorldSession* pWorld = GetWorldManager()->GetWorldByID(eventData.worldID);
    if(pWorld) {
        worldName = pWorld->worldName;
    }

    switch(subType) {
        case TCP_RENDER_REQUEST: {
            ServerInfo* pRenderServer = GetServerManager()->GetBestRenderServer();
            if(!pRenderServer) {
                GetServerManager()->SendRenderResult(false, eventData.userID, worldName, pNetServer->serverID);
                return;
            }
            eventData.FromVariant(data, false);

            GetServerManager()->SendRenderRequest(eventData.userID, eventData.worldID, pRenderServer->serverID);
            break;
        }

        case TCP_RENDER_RESULT: {
            eventData.FromVariant(data, true);

            PlayerSession* pPlayer = GetGameServer()->GetPlayerSessionByUserID(eventData.userID);
            if(!pPlayer) {
                return;
            }

            GetServerManager()->SendRenderResult(eventData.result, eventData.userID, worldName, pPlayer->serverID);
            break;
        }
    }
}
