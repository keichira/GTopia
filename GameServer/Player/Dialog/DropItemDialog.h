#pragma once
#include "Packet/PacketUtils.h"

class GamePlayer;
class InventoryItemInfo;

class DropItemDialog {
public:
    static void Request(GamePlayer* pPlayer, InventoryItemInfo* pInvItem);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<8>& packet);
};