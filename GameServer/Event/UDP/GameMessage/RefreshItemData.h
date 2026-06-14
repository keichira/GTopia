#pragma once

#include "../../../Player/GamePlayer.h"

class RefreshItemData {
public:
    static void Execute(GamePlayer* pPlayer, ParsedTextPacket<40>& packet);
};