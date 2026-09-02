#pragma once

#include "Network/NetClient.h"

void TCPEvent_PlayerEndSession(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader);
void TCPEvent_PlayerCheckSession(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader);