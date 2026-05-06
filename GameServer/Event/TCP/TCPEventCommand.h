#pragma once

#include "Network/NetClient.h"

class TCPEventCommand {
public:
    static void Execute(NetClient* pClient, VariantVector& data);
};