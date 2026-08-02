#include "TCPEvent_Server.h"
#include "../Context.h"
#include "../MasterBroadway.h"
#include "IO/Log.h"
#include "Utils/StringUtils.h"

void TCPEvent_Hello(NetClient* pClient, VariantVector& data)
{
    if (!pClient || data.size() < 2)
        return;

    string authKey = data[1].GetString();
    XorCipher(authKey, SOCKET_AUTH_SECRET_KEY);

    LOGGER_LOG_INFO("Received hello packet from Master");
    GetMasterBroadway()->SendAuthPacket(authKey);
}

void TCPEvent_Auth(NetClient* pClient, VariantVector& data)
{
    if (!pClient || data.size() < 2)
        return;

    bool authed = data[1].GetBool();
    GetMasterBroadway()->SetAuthState(authed ? BROADWAY_AUTH_SUCCESS : BROADWAY_AUTH_FAILED);
}

void TCPEvent_HeartBeat(NetClient* pClient, VariantVector& data)
{
    if (!pClient)
        return;
}

void TCPEvent_KillServer(NetClient* pClient, VariantVector& data)
{
    if (!pClient)
        return;

    GetContext()->Shutdown();
}