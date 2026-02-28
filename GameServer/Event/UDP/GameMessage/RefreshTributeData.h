#pragma once

#include "../../../Player/GamePlayer.h"

class RefreshTributeData {
public:
    static void Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet);
};