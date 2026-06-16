#pragma once

#include "../../../Player/GamePlayer.h"

class DialogReturn {
public:
    static void Execute(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};