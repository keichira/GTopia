#pragma once

#include "../../../Player/GamePlayer.h"

class Quit {
public:
    static void Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet);
};