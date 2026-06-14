#pragma once
#include "Packet/PacketUtils.h"

class GamePlayer;

class RegisterDialog {
public:
    static void Request(GamePlayer* pPlayer, const string& namePlaceholder = "", const string& passPlaceholder = "", const string& passVerifPlaceholder = "", const string& errorMsg = "");
    static void Handle(GamePlayer* pPlayer, ParsedTextPacket<40>& packet);
    static void Success(GamePlayer* pPlayer, const string& growID, const string& pass);
};