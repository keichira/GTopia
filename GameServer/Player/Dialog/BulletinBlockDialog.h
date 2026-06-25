#pragma once

#include "Packet/PacketUtils.h"

class GamePlayer;
class TileInfo;
class ItemInfo;
class World;

class BulletinBlockDialog {
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem);
    static void HandleFromCache(GamePlayer* pPlayer, uint32 worldInstanceID, int32 tileX, int32 tileY);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
    static void HandleDeleteEntry(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);

private:
    static void RequestDeleteEntry(GamePlayer* pPlayer, TileInfo* pTile, int32 index);
    static void SendBulletinDialog(GamePlayer* pPlayer, World* pWorld, TileInfo* pTile, ItemInfo* pItem);
};