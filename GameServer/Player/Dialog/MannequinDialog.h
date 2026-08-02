#pragma once

#include "Packet/PacketUtils.h"

class GamePlayer;
class TileInfo;

class MannequinDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile);
    static void RequestPutItem(GamePlayer* pPlayer, TileInfo* pTile, int32 itemID, bool fromDialog);
    static bool RequestRemoveItem(GamePlayer* pPlayer, TileInfo* pTile);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};