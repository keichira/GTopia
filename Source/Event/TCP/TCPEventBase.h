#pragma once

#include "../../Network/NetClient.h"
#include "../../Utils/Variant.h"
#include "../../Packet/GamePacket.h"

class TCPEventBase {
public:
    virtual ~TCPEventBase() {}
    virtual void Execute(NetClient* pClient, VariantVector& data) = 0;
};