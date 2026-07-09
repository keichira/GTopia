#pragma once

#include "../../../Player/GamePlayer.h"
#include "Packet/PacketUtils.h"

void GameMessage_EnterGame(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
void GameMessage_GrowID(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
void GameMessage_Quit(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
void GameMessage_QuitToExit(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
void GameMessage_RefreshTributeData(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
void GameMessage_JoinRequest(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
void GameMessage_Respawn(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
void GameMessage_RespawnSpkie(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);