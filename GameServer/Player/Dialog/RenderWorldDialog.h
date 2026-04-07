#pragma once
#include "Packet/PacketUtils.h"
#include "../GamePlayer.h"

class RenderWorldDialog {
public:
    static void Request(GamePlayer* pPlayer);
    static void Handle(GamePlayer* pPlayer);
    static void OnRendered(GamePlayer* pPlayer, const string& worldName);
};