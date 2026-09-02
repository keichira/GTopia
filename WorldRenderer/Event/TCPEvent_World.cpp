#include "TCPEvent_World.h"
#include "../WorldRendererManager.h"

void TCPEvent_RenderWorld(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    int32 subType = 0;
    if (!reader.Read<int32>(subType))
        return;

    if (subType != TCP_RENDER_REQUEST)
    {
        LOGGER_LOG_ERROR("HUH!? Master tried to send a result instead of request?");
        return;
    }

    uint32 playerUserID = 0;
    uint32 worldID = 0;

    if (!reader.Read<uint32>(playerUserID) || !reader.Read<uint32>(worldID))
        return;

    GetWorldRendererManager()->AddTask(playerUserID, worldID);
}