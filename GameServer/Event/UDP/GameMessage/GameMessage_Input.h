#pragma once

class GamePlayer;

#include "Packet/PacketUtils.h"

void GameMessage_DialogReturn(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
void GameMessage_SetSkin(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
void GameMessage_Input(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
void GameMessage_Wrench(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);