#include "TCPEventRenderWorld.h"
#include "../../Player/PlayerManager.h"
#include "../../Player/GamePlayer.h"
#include "IO/Log.h"

void TCPRenderWorldEventData::FromVariant(VariantVector& varVec)
{
    if(varVec.size() < 3) {
        return;
    }

    subType = varVec[1].GetINT();
    playerUserID = varVec[3].GetUINT();
}

void TCPEventRenderWorld::Execute(NetClient* pClient, VariantVector& data)
{
    TCPRenderWorldEventData eventData;
    eventData.FromVariant(data);

    if(eventData.subType != TCP_RENDER_RESULT) {
        LOGGER_LOG_ERROR("HUH!? Client tried to send a request instead of result? LOL");
        return;
    }

    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByUserID(eventData.playerUserID);
    if(!pPlayer) {
        LOGGER_LOG_WARN("Received player render world packet but player not found?");
        return;
    }

    pPlayer->HandleRenderWorld(std::move(data));
}