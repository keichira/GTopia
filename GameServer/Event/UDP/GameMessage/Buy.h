#pragma once

#include "../../../Player/GamePlayer.h"

class Buy {
public:
    static void Execute(GamePlayer* pPlayer, ParsedTextPacket<40>& packet);
};