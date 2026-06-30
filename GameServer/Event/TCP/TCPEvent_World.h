#pragma once

#include "Network/NetClient.h"

void TCPEvent_RenderWorld(NetClient* pClient, VariantVector& data);
void TCPEvent_WorldInit(NetClient* pClient, VariantVector& data);
void TCPEvent_WorldSendPlayer(NetClient* pClient, VariantVector& data);