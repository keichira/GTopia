#pragma once

#include "../../../Player/GamePlayer.h"

class EnterGame {
public:
    static void Execute(GamePlayer* pPlayer, ParsedTextPacket<40>& packet);
};