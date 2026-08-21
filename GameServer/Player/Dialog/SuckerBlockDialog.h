#pragma once

#include "Packet/PacketUtils.h"

class GamePlayer;
class TileInfo;

class SuckerBlockDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile);
    static void RequestAddItem(GamePlayer* pPlayer, TileInfo* pTile);
    static void RequestRetrieveItem(GamePlayer* pPlayer, TileInfo* pTile);

    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
    static void HandleAddItem(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
    static void HandleRetrieveItem(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};