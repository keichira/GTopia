#pragma once
#include "Packet/PacketUtils.h"

class GamePlayer;
class TileInfo;
class ItemInfo;

class CrystalBlockDialog {
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem);
};