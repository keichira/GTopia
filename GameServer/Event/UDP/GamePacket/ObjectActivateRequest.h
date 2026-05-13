#pragma once

#include "../../../Player/GamePlayer.h"
#include "../../../World/World.h"
#include "Packet/NetPacket.h"

class ObjectActivateRequest {
public:
    static void Execute(GamePlayer* pPlayer, World* pWorld, GameUpdatePacket* pPacket);
};