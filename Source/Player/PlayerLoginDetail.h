#pragma once

#include "../Precompiled.h"
#include "../Packet/PacketUtils.h"

class Player;

struct PlayerLoginDetail
{
    string requestedName = "";
    string tankIDName = "";
    string tankIDPass = "";

    float gameVersion;
    uint8 platformType;
    string country = "00";
    uint32 protocol = 0;
    
    string meta;
    int32 fhash;
    int32 hash;
    string mac;
    string rid;

    string wk;
    int32 zf;

    uint32 token;
    uint32 user;

    bool Serialize(ParsedTextPacket<25>& packet, Player* pPlayer, bool asGameServer);
};
