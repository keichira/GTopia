#pragma once

#include "Network/NetClient.h"
#include "Packet/GamePacket.h"

struct TCPRenderWorldEventData
{
    int32 subType = TCP_RENDER_RESULT;
    uint32 playerUserID = 0;

    void FromVariant(VariantVector& varVec);
};

class TCPEventRenderWorld {
public:
    static void Execute(NetClient* pClient, VariantVector& data);
};