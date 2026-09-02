#pragma once

#include "Network/NetClient.h"

void TCPEvent_PlayerCheckSession(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader);