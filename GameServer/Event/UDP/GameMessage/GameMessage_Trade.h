#pragma once

#include "../../../Player/GamePlayer.h"
#include "Packet/PacketUtils.h"

void GameMessage_ModTrade(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
void Gamemessage_RemTrade(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
void GameMessage_TradeAccept(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
void GameMessage_TradeCancel(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
void GameMessage_TradeLock(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
void GameMessage_TradeStarted(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);