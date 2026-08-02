#pragma once

#include "Packet/PacketUtils.h"

class GamePlayer;
class TileInfo;

class DressupDialog
{
public:
    static void Request(GamePlayer* pPlayer, TileInfo* pTile);
    static bool RequestPunch(GamePlayer* pPlayer, TileInfo* pTile);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
    static void HandleAsk(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};