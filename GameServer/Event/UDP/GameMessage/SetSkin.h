#pragma once

#include "../../../Player/GamePlayer.h"

class SetSkin {
public:
    static void Execute(GamePlayer* pPlayer, ParsedTextPacket<40>& packet);
};