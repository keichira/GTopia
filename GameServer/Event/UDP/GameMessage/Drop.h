#pragma once

#include "../../../Player/GamePlayer.h"

class Drop {
public:
    static void Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet);
};