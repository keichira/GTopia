#include "TCPEventAuth.h"
#include "../../Server/ServerManager.h"
#include "Utils/StringUtils.h"
#include "Utils/Timer.h"
#include "IO/Log.h"

void TCPEventAuth::Execute(NetClient* pClient, VariantVector& data)
{
    if(data.size() != 3) {
        pClient->status = SOCKET_CLIENT_CLOSE;
        return;
    }

    NetClientInfo* pClientInfo = (NetClientInfo*)pClient->data;

    if(pClientInfo->authKey != data[1].GetString()) {
        LOGGER_LOG_WARN("Failed to authorize server! closing connection...");
        pClient->status = SOCKET_CLIENT_CLOSE;
        return;
    }

    pClientInfo->authed = true;
    pClientInfo->lastHeartbeatTime = Time::GetSystemTime();

    VariantVector packet(2);
    packet[0] = TCP_PACKET_AUTH;
    packet[1] = true;

    GetServerManager()->AddServer(data[2].GetUINT(), pClient);
    pClient->Send(packet);
}