#pragma once

#include "../../../Player/GamePlayer.h"

class Trash {
public:
    static void Execute(GamePlayer* pPlayer, ParsedTextPacket<40>& packet);
};