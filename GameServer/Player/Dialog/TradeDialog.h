#pragma once

#include "Packet/PacketUtils.h"

class GamePlayer;
class InventoryItemInfo;

class TradeDialog {
public:
    static void Request(GamePlayer* pPlayer, InventoryItemInfo* pInvItem);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};