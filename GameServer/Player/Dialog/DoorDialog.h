#pragma once
#include "Packet/PacketUtils.h"

class GamePlayer;
class TileInfo;
class ItemInfo;

class DoorDialog {
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
    static void RequestPasswordDoor(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem);
    static void HandlePasswordReply(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};