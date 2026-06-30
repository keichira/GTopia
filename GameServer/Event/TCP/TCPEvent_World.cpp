#include "TCPEvent_World.h"
#include "../../Player/PlayerManager.h"
#include "../../World/WorldManager.h"

void TCPEvent_RenderWorld(NetClient* pClient, VariantVector& data)
{
    if (!pClient || data.size() < 4)
        return;

    int32 subType = data[1].GetINT();

    if (subType != TCP_RENDER_RESULT)
    {
        LOGGER_LOG_ERROR("HUH!? Client tried to send a request instead of result?");
        return;
    }

    uint32 playerUserID = data[3].GetUINT();

    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByUserID(playerUserID);
    if (pPlayer)
    {
        pPlayer->HandleRenderWorld(std::move(data));
    }
}

void TCPEvent_WorldInit(NetClient* pClient, VariantVector& data)
{
    if (!pClient || data.size() < 3)
        return;

    GetWorldManager()->HandleWorldInit(std::move(data));
}

void TCPEvent_WorldSendPlayer(NetClient* pClient, VariantVector& data)
{
    if (!pClient || data.size() < 4)
        return;

    GetWorldManager()->HandlePlayerJoin(std::move(data));
}
