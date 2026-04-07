#include "TCPEventAuth.h"
#include "../../Server/ServerManager.h"
#include "Utils/StringUtils.h"
#include "Utils/Timer.h"
#include "IO/Log.h"

void TCPAuthEventData::FromVariant(const VariantVector& varVec)
{
    if(varVec.size() < 4) {
        return;
    }

    authKey = varVec[1].GetString();
    serverID = varVec[2].GetUINT();
    serverType = varVec[3].GetINT();
}

void TCPEventAuth::Execute(NetClient* pClient, VariantVector& data)
{
    TCPAuthEventData eventData;
    eventData.FromVariant(data);

    NetServerInfo* pClientInfo = (NetServerInfo*)pClient->data;

    if(pClientInfo->authKey != eventData.authKey) {
        LOGGER_LOG_WARN("Failed to authorize server! closing connection...");
        pClient->status = SOCKET_CLIENT_CLOSE;
        return;
    }

    pClientInfo->authed = true;
    pClientInfo->lastHeartbeatTime.Reset();
    pClientInfo->serverID = eventData.serverID;

    GetServerManager()->AddServer(eventData.serverID, pClient, eventData.serverType);
    GetServerManager()->SendAuthPacket(true, eventData.serverID);
}