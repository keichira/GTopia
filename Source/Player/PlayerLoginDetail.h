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
    uint32 platformType;
    string country = "00";
    uint32 protocol = 0;
    uint32 loginMode = 0;
    
    string meta;
    uint32 fhash;
    int32 hash = 0;
    string mac;
    string rid;
    string sid;
    string gid;
    string vid;

    int32 zf = 0;
    uint32 fz = 0;

    uint32 token = 0;
    uint32 user = 0;

    bool Serialize(ParsedTextPacket<30>& packet, Player* pPlayer, bool asGameServer);
};
