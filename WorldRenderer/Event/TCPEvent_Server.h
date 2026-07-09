#pragma once

#include "Network/NetClient.h"

void TCPEvent_Hello(NetClient* pClient, VariantVector& data);
void TCPEvent_Auth(NetClient* pClient, VariantVector& data);
void TCPEvent_HeartBeat(NetClient* pClient, VariantVector& data);
void TCPEvent_KillServer(NetClient* pClient, VariantVector& data);