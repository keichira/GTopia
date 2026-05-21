#pragma once

#include "Network/NetClient.h"

class TCPEventWorldPlayerSession {
public:
    static void Execute(NetClient* pClient, VariantVector& data);
};