#pragma once

#include "../../../Player/GamePlayer.h"

class JoinRequest {
public:
    static void Execute(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};