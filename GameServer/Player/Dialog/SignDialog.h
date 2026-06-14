#pragma once

#include "Packet/PacketUtils.h"

class GamePlayer;
class TileInfo;

class SignDialog {
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<40>& packet);
};