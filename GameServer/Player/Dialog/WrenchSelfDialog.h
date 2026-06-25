#pragma once

#include "Packet/PacketUtils.h"

class GamePlayer;

class WrenchSelfDialog {
public:
    static void Request(GamePlayer* pPlayer);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};