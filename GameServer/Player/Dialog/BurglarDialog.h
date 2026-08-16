#pragma once

#include "Packet/PacketUtils.h"

class GamePlayer;
class TileInfo;

class BurglarDialog
{
public:
    static void RequestPunch(GamePlayer* pPlayer, TileInfo* pTile);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};