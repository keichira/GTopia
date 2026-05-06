#include "TCPEventKillServer.h"
#include "../../Server/ServerManager.h"
#include "../../World/WorldManager.h"
#include "../../Player/PlayerManager.h"

void TCPEventKillServer::Execute(NetClient* pClient, VariantVector& data)
{
    if(!pClient) {
        return;
    }

    ServerInfo* pServer = (ServerInfo*)pClient->data;
    if(!pServer) {
        return;
    }

    LOGGER_LOG_WARN("Killing server %d", pServer->serverID);
    GetServerManager()->RemoveServer(pServer->serverID);
}
