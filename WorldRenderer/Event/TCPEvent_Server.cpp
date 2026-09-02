#include "TCPEvent_Server.h"
#include "../Context.h"
#include "../MasterBroadway.h"
#include "IO/Log.h"
#include "Utils/StringUtils.h"

void TCPEvent_Hello(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    string authKey;
    if (!reader.ReadString(authKey))
        return;

    XorCipher(authKey, SOCKET_AUTH_SECRET_KEY);

    LOGGER_LOG_INFO("Received hello packet from Master");
    GetMasterBroadway()->SendAuthPacket(authKey);
}

void TCPEvent_Auth(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    bool authed = false;
    if (!reader.Read<bool>(authed))
        return;

    GetMasterBroadway()->SetAuthState(authed ? BROADWAY_AUTH_SUCCESS : BROADWAY_AUTH_FAILED);
}

void TCPEvent_HeartBeat(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;
}

void TCPEvent_KillServer(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    GetContext()->Shutdown();
}