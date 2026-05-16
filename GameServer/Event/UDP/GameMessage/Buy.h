#pragma once

#include "../../../Player/GamePlayer.h"

class Buy {
public:
    static void Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet);
};