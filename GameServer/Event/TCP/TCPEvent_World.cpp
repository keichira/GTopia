#include "TCPEvent_World.h"
#include "../../Player/PlayerManager.h"
#include "../../World/WorldManager.h"

void TCPEvent_RenderWorld(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    int32 subType = 0;
    if (!reader.Read<int32>(subType))
        return;

    if (subType != TCP_RENDER_RESULT)
    {
        LOGGER_LOG_ERROR("HUH!? Client tried to send a request instead of result?");
        return;
    }

    int32 result = 0;
    uint32 playerUserID = 0;

    if (!reader.Read<int32>(result) || !reader.Read<uint32>(playerUserID))
        return;

    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByUserID(playerUserID);
    if (pPlayer)
    {
        pPlayer->HandleRenderWorld(result, reader);
    }
}

void TCPEvent_WorldInit(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    GetWorldManager()->HandleWorldInit(reader);
}

void TCPEvent_WorldSendPlayer(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    GetWorldManager()->HandlePlayerJoin(reader);
}