#pragma once

#include "Network/NetClient.h"

void TCPEvent_Hello(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader);
void TCPEvent_Auth(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader);
void TCPEvent_HeartBeat(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader);
void TCPEvent_KillServer(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader);