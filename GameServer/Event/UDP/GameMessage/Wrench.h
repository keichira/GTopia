#pragma once

#include "../../../Player/GamePlayer.h"

class Wrench {
public:
    static void Execute(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};