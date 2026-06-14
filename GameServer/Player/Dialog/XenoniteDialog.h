#pragma once

#include "Packet/PacketUtils.h"

class GamePlayer;
class TileInfo;
class ItemInfo;

class XenoniteDialog {
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<40>& packet);
};