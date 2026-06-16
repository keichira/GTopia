#pragma once

#include "../../../Player/GamePlayer.h"

class Input {
public:
    static void Execute(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};