#pragma once

#include "Packet/PacketUtils.h"

class GamePlayer;
class TileInfo;
class ItemInfo;

class MailboxBlockDialog {
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem);
    static void HandleFromCache(GamePlayer* pPlayer, uint32 worldInstanceID, int32 tileX, int32 tileY);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<40>& packet);
};