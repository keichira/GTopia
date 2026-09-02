#pragma once

#include "Network/NetClient.h"

void TCPEvent_RenderWorld(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader);
void TCPEvent_WorldInit(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader);
void TCPEvent_WorldSendPlayer(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader);