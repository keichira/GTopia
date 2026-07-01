#pragma once
#include "Packet/PacketUtils.h"

class GamePlayer;
class TileInfo;
class ItemInfo;

class DonationBoxDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem);
    static void HandleFromCache(GamePlayer* pPlayer, uint32 worldInstanceID, int32 tileX, int32 tileY);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
    static void EmptyDonationBox(GamePlayer* pPlayer, TileInfo* pTile, bool allowDrop,
                                 const std::vector<bool>& selectedGifts = {});
    static void RequestDonatingItem(GamePlayer* pPlayer, TileInfo* pTile, int32 itemID);
    static void HandleGiveItem(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};