#pragma once

#include "../../../Player/GamePlayer.h"

class JoinRequest {
public:
    static void Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet);
};