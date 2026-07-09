#include "TCPEvent_World.h"
#include "../WorldRendererManager.h"

void TCPEvent_RenderWorld(NetClient* pClient, VariantVector& data)
{
    if (!pClient || data.size() < 4)
        return;

    int32 subType = data[1].GetINT();

    if (subType != TCP_RENDER_REQUEST)
    {
        LOGGER_LOG_ERROR("HUH!? Master tried to send a result instead of request?");
        return;
    }

    uint32 playerUserID = data[2].GetUINT();
    uint32 worldID = data[3].GetUINT();

    GetWorldRendererManager()->AddTask(playerUserID, worldID);
}