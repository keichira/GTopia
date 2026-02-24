#pragma once

#include "Network/NetClient.h"

class TCPEventHello {
public:
    static void Execute(NetClient* pClient, VariantVector& data);
};