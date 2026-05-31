#pragma once
#include "Packet/PacketUtils.h"

class GamePlayer;
class TileInfo;

class OuijaBoardDialog {
private:
    enum class eOuijaItemInfoType
    {
        MAIN,
        NAME,
        AMOUNT,
        RARITY,
        SEED
    };

public:
    static void RequestMain(GamePlayer* pPlayer, TileInfo* pTile);
    static void RequestItemInfo(GamePlayer* pPlayer, TileInfo* pTile, int32 itemIndex, eOuijaItemInfoType type);
    static void RequestCommand(GamePlayer* pPlayer, TileInfo* pTile);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<8>& packet);
};