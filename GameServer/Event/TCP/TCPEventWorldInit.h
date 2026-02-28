#pragma once

#include "Network/NetClient.h"

class TCPEventWorldInit {
public:
    static void Execute(NetClient* pClient, VariantVector& data);
};