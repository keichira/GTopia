#pragma once

#include "../../../Player/GamePlayer.h"

class QuitToExit {
public:
    static void Execute(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
};