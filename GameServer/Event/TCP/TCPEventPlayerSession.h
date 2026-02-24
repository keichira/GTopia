#pragma once

#include "Network/NetClient.h"

class TCPEventPlayerSession {
public:
    static void Execute(NetClient* pClient, VariantVector& data);
};