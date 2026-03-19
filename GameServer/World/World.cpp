#include "World.h"
#include "IO/Log.h"
#include "../Player/GamePlayer.h"
#include "Packet/NetPacket.h"
#include "Item/ItemInfoManager.h"

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

    pPlayer->SetJoinWorld(false);
    pPlayer->SetCurrentWorld(m_worldID);
    m_players.push_back(pPlayer);

    TileInfo* pMainDoorTile = GetTileManager()->GetTile(KEY_TILE_MAIN_DOOR);
    if(!pMainDoorTile) {
        pPlayer->SetWorldPos(0, 0);
        pPlayer->SetRespawnPos(0, 0);
    }
    else {
        Vector2Int mainDoorPos = pMainDoorTile->GetPos();
        pPlayer->SetWorldPos(mainDoorPos.x * 32, mainDoorPos.y * 32);
        pPlayer->SetRespawnPos(mainDoorPos.x * 32, mainDoorPos.y * 32);
    }

    MemoryBuffer memSize;
    Serialize(memSize, true, false);
    
    uint32 worldMemSize = memSize.GetOffset();
    uint8* pWorldData = new uint8[worldMemSize];

    MemoryBuffer memBuffer(pWorldData, worldMemSize);
    Serialize(memBuffer, true, false);

    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_SEND_MAP_DATA;
    packet.netID = -1;
    packet.flags |= NET_GAME_PACKET_FLAGS_EXTENDED;
    packet.extraDataSize = worldMemSize;
    SendENetPacketRaw(NET_MESSAGE_GAME_PACKET, &packet, sizeof(GameUpdatePacket), pWorldData, pPlayer->GetPeer());
    SAFE_DELETE_ARRAY(pWorldData);

    pPlayer->SendOnSpawn(pPlayer->GetSpawnData(true));

    for(auto& pWorldPlayer : m_players) {
        if(pWorldPlayer) {
            pWorldPlayer->SendCharacterState(pPlayer);
            pPlayer->SendCharacterState(pWorldPlayer);

            pWorldPlayer->SendOnSetClothing(pPlayer);
            pPlayer->SendOnSetClothing(pWorldPlayer);

            if(pPlayer != pWorldPlayer) {
                pPlayer->SendOnSpawn(pWorldPlayer->GetSpawnData(false));
                pWorldPlayer->SendOnSpawn(pPlayer->GetSpawnData(false));
            }
        }
    }

    return true;
}

void World::PlayerLeaverWorld(GamePlayer* pPlayer)
{
    for(int16 i = m_players.size() - 1; i >= 0; --i) {
        GamePlayer* pWorldPlayer = m_players[i];

        pWorldPlayer->SendOnRemove(pPlayer->GetNetID());

        if(pWorldPlayer == pPlayer) {
            pWorldPlayer = m_players.back();
            m_players.pop_back();

            pPlayer->SetCurrentWorld(0);
        }
    }

    if(m_players.empty()) {
        m_worldOfflineTime.Reset();
    }
}

void World::SendSkinColorUpdateToAll(GamePlayer* pPlayer)
{
    if(!pPlayer) {
        return;
    }

    uint32 skinColor = pPlayer->GetCharData().GetSkinColor(true);
    for(auto& pWorldPlayer: m_players) {
        if(pWorldPlayer) {
            pWorldPlayer->SendOnChangeSkin(skinColor, pPlayer);
        }
    }
}

void World::SendSetCharPacketToAll(GamePlayer* pPlayer)
{
    if(!pPlayer) {
        return;
    }

    for(auto& pWorldPlayer : m_players) {
        if(pWorldPlayer) {
            pWorldPlayer->SendCharacterState(pPlayer);
        }
    }
}

void World::SendClothUpdateToAll(GamePlayer* pPlayer)
{
    if(!pPlayer) {
        return;
    }

    for(auto& pWorldPlayer : m_players) {
        if(pWorldPlayer) {
            pWorldPlayer->SendOnSetClothing(pPlayer);
        }
    }
}

void World::SendParticleEffectToAll(float coordX, float coordY, uint32 particleType, float particleSize, int32 delay)
{
    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_SEND_PARTICLE_EFFECT;
    packet.posX = coordX;
    packet.posY = coordY;
    packet.particleEffectType = (float)particleType;
    packet.particleEffectSize = particleSize;
    packet.delay = delay;

    for(auto& pWorldPlayer : m_players) {
        SendGamePacketToAll(&packet);
    }
}

void World::SendTileUpdate(uint16 tileX, uint16 tileY)
{
    TileInfo* pTile = GetTileManager()->GetTile(tileX, tileY);
    if(!pTile) {
        return;
    }

    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_SEND_TILE_UPDATE_DATA;
    packet.tileX = tileX;
    packet.tileY = tileY;
    packet.flags |= NET_GAME_PACKET_FLAGS_EXTENDED;

    MemoryBuffer memSizeBuf;
    pTile->Serialize(memSizeBuf, true, false, GetWorldVersion());

    uint32 memSize = memSizeBuf.GetOffset();
    packet.extraDataSize = memSize;

    uint8* pTileData = new uint8[memSize];
    MemoryBuffer memBuffer(pTileData, memSize);
    pTile->Serialize(memBuffer, true, false, GetWorldVersion());

    SendGamePacketToAll(&packet, nullptr, pTileData);

    SAFE_DELETE_ARRAY(pTileData);
}

void World::SendTileApplyDamage(uint16 tileX, uint16 tileY, int32 damage, int32 netID, GamePlayer* pPlayer)
{
    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_TILE_APPLY_DAMAGE;
    packet.tileX = tileX;
    packet.tileY = tileY;
    packet.tileDamage = damage;
    packet.netID = netID;

    if(pPlayer) {
        SendENetPacketRaw(NET_MESSAGE_GAME_PACKET, &packet, sizeof(GameUpdatePacket), nullptr, pPlayer->GetPeer());
    }
    else {
        SendGamePacketToAll(&packet);
    }
}

void World::PlaySFXForEveryone(const string& fileName, int32 delay)
{
    for(auto& pWorldPlayer : m_players) {
        if(pWorldPlayer) {
            pWorldPlayer->PlaySFX(fileName, delay);
        }
    }
}

void World::SendGamePacketToAll(GameUpdatePacket* pPacket, GamePlayer* pExceptMe, uint8* pExtraData)
{
    for(auto& pWorldPlayer : m_players) {
        if(pWorldPlayer) {
            if(pExceptMe && (pWorldPlayer == pExceptMe)) {
                continue;
            }

            SendENetPacketRaw(NET_MESSAGE_GAME_PACKET, pPacket, sizeof(GameUpdatePacket), pExtraData, pWorldPlayer->GetPeer());
        }
    }
}

void World::SendCurrentWeatherToAll()
{
    for(auto& pWorldPlayer : m_players) {
        if(pWorldPlayer) {
            pWorldPlayer->SendOnSetCurrentWeather(GetCurrentWeather());
        }
    }
}

void World::HandleTilePackets(GameUpdatePacket* pGamePacket)
{
    if(!pGamePacket) {
        return;
    }

    switch(pGamePacket->type) {
        case NET_GAME_PACKET_TILE_CHANGE_REQUEST: {
            TileInfo* pTile = GetTileManager()->GetTile(pGamePacket->tileX, pGamePacket->tileY);
            if(!pTile) {
                return;
            }
    
            ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pGamePacket->itemID);
            if(!pItem) {
                return;
            }

            if(pItem->IsBackground()) {
                pTile->SetBG(pItem->id);
            }
            else if(pItem->type == ITEM_TYPE_SEED) {

            }
            else if(pItem->type == ITEM_TYPE_FIST) {
                if(pTile->GetFG() != ITEM_ID_BLANK) {
                    pTile->SetFG(ITEM_ID_BLANK, GetTileManager());
                }
                else {
                    pTile->SetBG(ITEM_ID_BLANK);
                }
            }
            else {
                if(pItem->IsBackground()) {
                    pTile->SetBG(pItem->id);
                }
                else {
                    pTile->SetFG(pItem->id, GetTileManager());
                }
            }

            if(pItem->IsBackground()) {
                if(pTile->HasFlag(TILE_FLAG_HAS_EXTRA_DATA)) {
                    if(pGamePacket->HasFlag(NET_GAME_PACKET_FLAGS_FACINGLEFT)) {
                        pTile->SetFlag(TILE_FLAG_FLIPPED_X);
                    }
                    else {
                        pTile->RemoveFlag(TILE_FLAG_FLIPPED_X);
                    }
                }
            }
            else {
                /**
                 * 
                 * seed check
                 * 
                 */

                if(pItem->HasFlag(ITEM_FLAG_FLIPPABLE)) {
                    if(pGamePacket->HasFlag(NET_GAME_PACKET_FLAGS_FACINGLEFT)) {
                        pTile->SetFlag(TILE_FLAG_FLIPPED_X);
                    }
                    else {
                        pTile->RemoveFlag(TILE_FLAG_FLIPPED_X);
                    }
                }
            }
            break;
        }
    }
}
