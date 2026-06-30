#pragma once

#include "Network/NetClient.h"

void TCPEvent_PlayerEndSession(NetClient* pClient, VariantVector& data);
void TCPEvent_PlayerCheckSession(NetClient* pClient, VariantVector& data);