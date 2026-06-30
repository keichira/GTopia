#pragma once

#include "../../../Player/GamePlayer.h"
#include "Packet/PacketUtils.h"

void GameMessage_Drop(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
void GameMessage_Trash(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
void GameMessage_RefreshItemData(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
void GameMessage_Buy(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
void GameMessage_Store(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);