#include "TCPEventRenderWorld.h"
#include "../../Server/ServerManager.h"
#include "../../Player/PlayerManager.h"
#include "../../World/WorldManager.h"

void TCPEventRenderWorldData::FromVariant(VariantVector& varVec, bool forResult)
{
    if(varVec.size() < 2) {
        return;
    }

    if(forResult) 
    {
        result = varVec[2].GetINT();
        userID = varVec[3].GetUINT();
        worldID = varVec[4].GetUINT();
    }
    else 
    {
        userID = varVec[2].GetUINT();
        worldID = varVec[3].GetUINT();
    }
}

void TCPEventRenderWorld::Execute(NetClient* pClient, VariantVector& data)
{
    if(!pClient)
        return;

    if(data.size() < 2)
        return;

    ServerInfo* pServer = (ServerInfo*)pClient->data;
    if(!pServer)
        return;

    TCPEventRenderWorldData eventData;
    int32 subType = data[1].GetINT();

    eventData.FromVariant(data, subType == TCP_RENDER_RESULT);

    string worldName;
    WorldSession* pWorld = GetWorldManager()->GetWorldByInstanceID(eventData.worldID);
    if(pWorld) 
    {
        worldName = pWorld->worldName;
    }

    switch(subType) 
    {
        case TCP_RENDER_REQUEST: 
        {
            ServerInfo* pRenderServer = GetServerManager()->GetBestRenderServer();
            if(!pRenderServer) 
            {
                GetServerManager()->SendRenderResult(pServer, TCP_RESULT_FAIL, eventData.userID, worldName);
                return;
            }

            GetServerManager()->SendRenderRequest(pRenderServer, eventData.userID, eventData.worldID);
            break;
        }

        case TCP_RENDER_RESULT:
        {
            PlayerSession* pPlayer = GetPlayerManager()->GetSessionByID(eventData.userID);
            if(!pPlayer)
                return;

            GetServerManager()->SendRenderResult(pServer, eventData.result, eventData.userID, worldName);
            break;
        }
    }
}
