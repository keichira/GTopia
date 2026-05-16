#pragma once

#include "../../../Player/GamePlayer.h"

class Store {
public:
    static void Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet);
};