#pragma once

#include "Packet/PacketUtils.h"

class GamePlayer;
class TileInfo;

class SpotlightDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};