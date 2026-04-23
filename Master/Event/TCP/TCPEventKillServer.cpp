#include "TCPEventKillServer.h"
#include "../../Server/ServerManager.h"
#include "../../World/WorldManager.h"
#include "../../Player/PlayerManager.h"

void TCPKillServerEventData::FromVariant(VariantVector& varVec)
{
    if(varVec.size() < 2) {
        return;
    }

    serverID = varVec[1].GetUINT();
}

void TCPEventKillServer::Execute(NetClient* pClient, VariantVector& data)
{
    TCPKillServerEventData eventData;
    eventData.FromVariant(data);

    LOGGER_LOG_WARN("Killing server %d", eventData.serverID);
    GetServerManager()->RemoveServer(eventData.serverID);
    GetWorldManager()->RemoveWorldsWithServerID(eventData.serverID);
    GetPlayerManager()->EndSessionsByServer(eventData.serverID);
}
