#pragma once
#include "Packet/PacketUtils.h"
#include "../GamePlayer.h"

class RenderWorldDialog {
public:
    static void Request(GamePlayer* pPlayer);
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
    static void OnRendered(GamePlayer* pPlayer, const string& worldName);
};