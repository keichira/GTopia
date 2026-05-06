#include "TCPEventHeartBeat.h"
#include "../../Player/PlayerManager.h"
#include "../../Server/MasterBroadway.h"
#include "Packet/GamePacket.h"

void TCPHeartBeatEventData::FromVariant(VariantVector& varVec)
{
    if(varVec.size() < 2)
        return;

    playerCount = varVec[1].GetUINT();
}

void TCPEventHeartBeat::Execute(NetClient* pClient, VariantVector& data)
{
    if(!pClient)
        return;

    TCPHeartBeatEventData eventData;
    eventData.FromVariant(data);

    GetPlayerManager()->SetTotalPlayerCount(eventData.playerCount);
    GetMasterBroadway()->SendHeartBeat();
}
