#include "TCPEventHello.h"

void TCPHelloEventData::FromVariant(const VariantVector& varVec)
{
    if(varVec.size() < 2)
        return;

    authKey = varVec[1].GetString();
}

void TCPEventHello::Execute(NetClient* pClient, VariantVector& data)
{
    TCPHelloEventData eventData;
    eventData.FromVariant(data);

    GetMasterBroadway()->SendAuthPacket(eventData.authKey);
}