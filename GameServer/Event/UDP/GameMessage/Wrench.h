#pragma once

#include "../../../Player/GamePlayer.h"

class Wrench {
public:
    static void Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet);
};