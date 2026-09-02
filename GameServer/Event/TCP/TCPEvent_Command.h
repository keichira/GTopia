#pragma once

#include "Network/NetClient.h"

void TCPEvent_Command(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader);
void TCPEvent_Command_SetRole(TCPPacketReader& reader);