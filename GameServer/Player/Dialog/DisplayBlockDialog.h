#pragma once

#include "Packet/PacketUtils.h"

class GamePlayer;
class TileInfo;

class DisplayBlockDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile);
    static void RequestPutItem(GamePlayer* pPlayer, TileInfo* pTile, int32 itemID);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};