#include "TCPEvent_Server.h"
#include "../../Context.h"
#include "../../Player/PlayerManager.h"
#include "../../Server/MasterBroadway.h"
#include "IO/Log.h"
#include "Utils/StringUtils.h"

#include "IO/File.h"

void TCPEvent_Hello(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    string authKey;
    if (!reader.ReadString(authKey))
        return;

    // XorCipher(authKey, SOCKET_AUTH_SECRET_KEY);

    LOGGER_LOG_INFO("Received hello packet from Master");
    GetMasterBroadway()->SendAuthPacket(authKey);
}

void TCPEvent_Auth(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    uint8 authed = 0;
    if (!reader.Read<uint8>(authed))
        return;

    GetMasterBroadway()->SetAuthState(authed == 1 ? BROADWAY_AUTH_SUCCESS : BROADWAY_AUTH_FAILED);
}

void TCPEvent_HeartBeat(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    uint32 playerCount = 0;
    if (!reader.Read<uint32>(playerCount))
        return;

    GetPlayerManager()->SetTotalPlayerCount(playerCount);
    GetMasterBroadway()->SendHeartBeat();
}

void TCPEvent_KillServer(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    GetContext()->Shutdown();
}