#include "World.h"
#include "IO/Log.h"
#include "../Player/GamePlayer.h"
#include "Packet/NetPacket.h"

#include "IO/File.h"

World::World()
: m_worldID(0)
{
}

World::~World()
{
}

bool World::PlayerJoinWorld(GamePlayer* pPlayer)
{
    if(!pPlayer) {
        return false;
    }

    m_players.push_back(pPlayer);

    MemoryBuffer memSize;
    Serialize(memSize, true, false);
    
    uint32 worldMemSize = memSize.GetOffset();
    uint8* pWorldData = new uint8[worldMemSize];
    memset(pWorldData, 0, worldMemSize);

    MemoryBuffer memBuffer(pWorldData, worldMemSize);
    Serialize(memBuffer, true, false);

    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_SEND_MAP_DATA;
    packet.netID = -1;
    packet.flags |= NET_GAME_PACKET_FLAGS_EXTENDED;
    packet.extraDataSize = worldMemSize;
    SendENetPacketRaw(NET_MESSAGE_GAME_PACKET, &packet, sizeof(GameUpdatePacket), pWorldData, pPlayer->GetPeer());
    SAFE_DELETE_ARRAY(pWorldData);

    //todo onspawn
    /*VariantVector data(2);
    data[0] = "OnSpawn";
    data[1] = "spawn|avatar\nnetID|6\nuserID|6\ncolrect|0|0|20|30\nposXY|0|0\nname|```#@ddddaaaa\ncountry|rt\ninvis|0\nmstate|1\nsmstate|1\nonlineID|\ntype|local\n";
    pPlayer->SendCallFunctionPacket(data);*/

    return true;
}
