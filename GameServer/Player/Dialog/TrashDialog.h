#pragma once

#include "Packet/PacketUtils.h"

class GamePlayer;

class TrashDialog
{
public:
    static void Request(GamePlayer* pPlayer, uint16 itemID);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
    static void HandleUntradeable(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};