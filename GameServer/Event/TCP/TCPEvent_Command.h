#pragma once

#include "Network/NetClient.h"

void TCPEvent_Command(NetClient* pClient, VariantVector& data);
void TCPEvent_Command_SetRole(VariantVector&& data);