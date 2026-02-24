#pragma once

#include "Network/NetClient.h"

class TCPEventAuth {
public:
    static void Execute(NetClient* pClient, VariantVector& data);
};