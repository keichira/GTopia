#include "World.h"
#include "IO/Log.h"
#include "Packet/NetPacket.h"
#include "Math/Rect.h"
#include "Math/Math.h"
#include "../Context.h"
#include "IO/File.h"
#include "../Player/PlayerManager.h"
#include "WorldManager.h"
#include "../Server/MasterBroadway.h"
#include "Utils/GrowUtils.h"
#include "../Server/UserCacheManager.h"
#include "Math/WeightRand.h"

World::World()
: m_databaseID(0), m_state(WORLD_STATE_LOADING), m_instanceID(0)
{
    m_pNpcManager = new WorldNPCManager(this);
}

World::~World()
{
    SAFE_DELETE(m_pNpcManager);
}

bool World::InitWorld()
{
    if(m_databaseID == 0)
        return false;

    string path = GetContext()->GetGameConfig()->worldSavePath
        + "/world_" + ToString(m_databaseID) + ".bin";

    File file;
    if(!file.Open(path))
        return false;

    uint32 fileSize = file.GetSize();
    uint8* pData = new uint8[fileSize];

    if(file.Read(pData, fileSize) != fileSize)
    {
        file.Close();
        SAFE_DELETE_ARRAY(pData);
        return false;
    }

    file.Close();

    MemoryBuffer memBuffer(pData, fileSize);
    Serialize(memBuffer, false, true);

    SAFE_DELETE_ARRAY(pData);
    return true;
}

void World::SaveToDatabaseCB(QueryTaskResult&& result)
{
    World* pWorld = GetWorldManager()->GetWorldByInstanceID(result.ownerID);
    if(!pWorld)
        return;

    File file;
    string worldSavePath = GetContext()->GetGameConfig()->worldSavePath + "/world_" + ToString(pWorld->GetDatabaseID()) + ".bin";
    if(!file.Open(worldSavePath, FILE_MODE_WRITE))
        return;

    uint32 worldMemSize = pWorld->GetMemEstimate(true);
    uint8* pWorldData = new uint8[worldMemSize];

    MemoryBuffer memBuffer(pWorldData, worldMemSize);
    pWorld->Serialize(memBuffer, true, true);

    if(file.Write(pWorldData, worldMemSize) != worldMemSize) 
    {
        file.Close();
        SAFE_DELETE_ARRAY(pWorldData);
        return;
    }

    file.Close();
    SAFE_DELETE_ARRAY(pWorldData);
}

void World::SaveToDatabase()
{
    QueryRequest req = WorldDB::Save(GetWorlName(), GetDatabaseID(), GetInstanceID());
    req.callback = &World::SaveToDatabaseCB;

    DatabaseWorldExec(GetContext()->GetDatabasePool(), req);
}

void World::Update()
{
    if(m_state != WORLD_STATE_READY)
        return;

    if(m_players.size() > 0) 
    {
        m_worldOfflineTime.Reset();
    }
    else 
    {
        if(m_worldOfflineTime.GetElapsedTime() >= 3600 * 1000) 
        {
            SetState(WORLD_STATE_DELETE);
        }
    }

    if(m_players.size() > 0 && m_pNpcManager)
    {
        m_pNpcManager->Update();
    }

    if(m_worldLastSaveTime.GetElapsedTime() >= 40 * 60 * 1000) 
    {
        SaveToDatabase();
    }
}

bool World::ExportWorld(const string& name)
{
    if(name.empty())
        return false;

    File file;
    if(!file.Open(GetProgramPath() + "/" + name + ".bin", FILE_MODE_WRITE))
        return false;

    uint32 worldMemEst = GetMemEstimate(true);
    uint8* pData = new uint8[worldMemEst];
    
    MemoryBuffer memBuffer(pData, worldMemEst);
    Serialize(memBuffer, true, true);

    if(file.Write(pData, worldMemEst) != worldMemEst)
    {
        file.Close();
        SAFE_DELETE_ARRAY(pData);
        return false;
    }

    file.Close();
    SAFE_DELETE_ARRAY(pData);
    return true;
}

bool World::ImportWorld(const string& name)
{
    if(name.empty())
        return false;

    string worldName = GetWorlName();
    if(worldName.empty())
        return false;

    File file;
    if(!file.Open(GetProgramPath() + "/" + name + ".bin"))
        return false;

    uint32 worldMemSize = file.GetSize();
    uint8* pData = new uint8[worldMemSize];

    if(file.Read(pData, worldMemSize) != worldMemSize)
    {
        file.Close();
        SAFE_DELETE_ARRAY(pData);
        return false;
    }

    auto players = m_players;
    RemoveAllPlayers();

    MemoryBuffer memBuffer(pData, worldMemSize);
    Serialize(memBuffer, false, true);
    SAFE_DELETE_ARRAY(pData);
    file.Close();

    SetName(worldName);

    for(auto& pPlayer : players)
    {
        if(!pPlayer)
            continue;

        AddPlayer(pPlayer, false);
    }

    return true;
}

void World::AddPlayer(GamePlayer* pPlayer, bool newJoin)
{
    if(!pPlayer)
        return;

    pPlayer->SetJoiningWorld(false);
    pPlayer->SetCurrentWorld(m_instanceID);
    m_players.push_back(pPlayer);

    TileInfo* pMainDoorTile = GetTileManager()->GetKeyTile(KEY_TILE_MAIN_DOOR);
    if(!pMainDoorTile) 
    {
        pPlayer->SetWorldPos(0, 0);
        pPlayer->SetRespawnPos(0, 0);
    }
    else 
    {
        Vector2Int mainDoorPos = pMainDoorTile->GetPos();
        pPlayer->SetWorldPos(mainDoorPos.x * 32, mainDoorPos.y * 32);
        pPlayer->SetRespawnPos(mainDoorPos.x * 32, mainDoorPos.y * 32);
    }
    
    uint32 worldMemSize = GetMemEstimate(false);
    uint8* pWorldData = new uint8[worldMemSize];

    MemoryBuffer memBuffer(pWorldData, worldMemSize);
    Serialize(memBuffer, true, false);

    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_SEND_MAP_DATA;
    packet.field_4 = -1;
    packet.flags |= GAME_PACKET_FLAG_EXTENDED_DATA;
    packet.extraDataSize = worldMemSize;
    SendENetPacketRaw(NET_MESSAGE_GAME_PACKET, &packet, sizeof(GameUpdatePacket), pWorldData, pPlayer->GetPeer());
    SAFE_DELETE_ARRAY(pWorldData);

    pPlayer->SendOnSpawn(pPlayer->GetSpawnData(true));
    pPlayer->SendOnSetClothing();
    pPlayer->SendCharacterState();

    if(newJoin)
    {
        GetMasterBroadway()->SendPlayerJoinedWorld(pPlayer->GetUserID(), GetInstanceID());    
    }

    string playerDisplayName = pPlayer->GetDisplayName(true);
    string joinNotifyOtherMsg = "`5<" + playerDisplayName + " `5entered, `w" + ToString(GetPlayerCount() - 1) + " `5others here``>";

    for(auto& pWorldPlayer : m_players) 
    {
        if(pWorldPlayer && pWorldPlayer != pPlayer) 
        {
            pPlayer->SendOnSpawn(pWorldPlayer->GetSpawnData(false));
            pPlayer->SendOnSetClothing(pWorldPlayer);
            pPlayer->SendCharacterState(pWorldPlayer);

            pWorldPlayer->SendOnSpawn(pPlayer->GetSpawnData(false));
            pWorldPlayer->SendOnSetClothing(pPlayer);
            pWorldPlayer->SendCharacterState(pPlayer);
            pWorldPlayer->SendOnTalkBubble(joinNotifyOtherMsg, true, pPlayer);
            pWorldPlayer->SendOnConsoleMessage(joinNotifyOtherMsg);
        }
    }

    string worldSituationMsg;

    WorldTileManager* pTileMgr = GetTileManager();
    if(TileInfo* pTile = pTileMgr->GetKeyTile(KEY_TILE_PUNCH_JAMMER); pTile && pTile->HasFlag(TILE_FLAG_IS_ON)) 
    {
        if(!worldSituationMsg.empty()) worldSituationMsg += ", ";
        worldSituationMsg += "`2NOPUNCH``";
    }

    if(TileInfo* pTile = pTileMgr->GetKeyTile(KEY_TILE_ZOMBIE_JAMMER); pTile && pTile->HasFlag(TILE_FLAG_IS_ON)) 
    {
        if(!worldSituationMsg.empty()) worldSituationMsg += ", ";
        worldSituationMsg += "`2IMMUNE``";
    }

    if(TileInfo* pTile = pTileMgr->GetKeyTile(KEY_TILE_SIGNAL_JAMMER); pTile && pTile->HasFlag(TILE_FLAG_IS_ON)) 
    {
        if(!worldSituationMsg.empty()) worldSituationMsg += ", ";
        worldSituationMsg += "`4JAMMED``";
    }

    if(TileInfo* pTile = pTileMgr->GetKeyTile(KEY_TILE_ANTIGRAVITY); pTile && pTile->HasFlag(TILE_FLAG_IS_ON)) 
    {
        if(!worldSituationMsg.empty()) worldSituationMsg += ", ";
        worldSituationMsg += "`2ANTIGRAVITY``";
    }

    string worldEnterMsg = "World `w" + GetWorlName() + "`o ";
    if(!worldSituationMsg.empty()) 
    {
        worldEnterMsg += "`0[``" + worldSituationMsg + "`0] ";
    }
    worldEnterMsg += "`oentered, There are `w" + ToString(GetPlayerCount() - 1) + "`o other people here, `w" + ToString(GetPlayerManager()->GetTotalPlayerCount()) + " `oonline.";

    pPlayer->SendOnConsoleMessage(worldEnterMsg);

    TileInfo* pWorldLock = pTileMgr->GetKeyTile(KEY_TILE_WORLD_LOCK);
    if(pWorldLock) 
    {
        TileExtra_Lock* pExtra = pWorldLock->GetExtra<TileExtra_Lock>();
        if(pExtra) 
        {
            /**
             * InactivityManager! :)
             */
        }
    }
}

void World::PlayerLeaveWorld(GamePlayer* pPlayer, bool hardLeave)
{
    if(!pPlayer)
        return;

    int32 playerIdx = -1;

    for(uint16 i = 0; i < m_players.size(); ++i) 
    {
        GamePlayer* pWorldPlayer = m_players[i];

        pWorldPlayer->SendOnRemove(pPlayer->GetNetID());

        if(pWorldPlayer == pPlayer) {
            playerIdx = i;
        }
    }

    if(playerIdx != -1) 
    {
        m_players[playerIdx] = m_players.back();
        m_players.pop_back();

        pPlayer->SetCurrentWorld(0);

        if(hardLeave)
        {
            GetMasterBroadway()->SendPlayerLeftWorld(pPlayer->GetUserID(), GetInstanceID());
        }
    }

    if(m_players.empty()) 
    {
        m_worldOfflineTime.Reset();
    }
}

void World::RemoveAllPlayers()
{
    for(auto& pPlayer : m_players)
    {
        if(!pPlayer)
            continue;

        PlayerLeaveWorld(pPlayer, false);
    }
}

void World::ReconnectPlayers()
{
    auto players = m_players;

    RemoveAllPlayers();

    for(auto& pPlayer : players)
    {
        if(!pPlayer)
            continue;

        AddPlayer(pPlayer, false);
    }
}

void World::SendSkinColorUpdateToAll(GamePlayer* pPlayer)
{
    if(!pPlayer)
        return;

    uint32 skinColor = pPlayer->GetModController().GetSkinColor();
    for(auto& pWorldPlayer: m_players) 
    {
        if(pWorldPlayer) 
        {
            pWorldPlayer->SendOnChangeSkin(skinColor, pPlayer);
        }
    }
}

void World::SendTalkBubbleAndConsoleToAll(const string& message, bool stackBubble, GamePlayer* pPlayer)
{
    for(auto& pWorldPlayer : m_players) {
        if(pWorldPlayer) {
            pWorldPlayer->SendOnConsoleMessage(message);
            pWorldPlayer->SendOnTalkBubble(message, stackBubble, pPlayer);
        }
    }
}

void World::SendConsoleMessageToAll(const string& message)
{
    for(auto& pWorldPlayer : m_players) {
        if(pWorldPlayer) {
            pWorldPlayer->SendOnConsoleMessage(message);
        }
    }
}

void World::SendNameChangeToAll(GamePlayer* pPlayer)
{
    if(!pPlayer) {
        return;
    }

    string playerName = pPlayer->GetDisplayName(true);
    for(auto& pWorldPlayer : m_players) {
        if(pWorldPlayer) {
            pWorldPlayer->SendOnNameChanged(playerName, pPlayer);
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
    packet.field_8.x = coordX;
    packet.field_8.y = coordY;
    packet.field_9.x = (float)particleType;
    packet.field_9.y = particleSize;
    packet.field_7 = delay;

    for(auto& pWorldPlayer : m_players) {
        SendGamePacketToAll(&packet);
    }
}

void World::SendTileUpdate(TileInfo* pTile, GamePlayer* pPlayer)
{
    if(!pTile) {
        return;
    }

    Vector2Int& vTilePos = pTile->GetPos();
    SendTileUpdate(vTilePos.x, vTilePos.y, pPlayer);
}

void World::SendTileUpdate(uint16 tileX, uint16 tileY, GamePlayer* pPlayer)
{
    TileInfo* pTile = GetTileManager()->GetTile(tileX, tileY);
    if(!pTile) {
        return;
    }

    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_SEND_TILE_UPDATE_DATA;
    packet.field_11 = tileX;
    packet.field_12 = tileY;
    packet.flags |= GAME_PACKET_FLAG_EXTENDED_DATA;

    MemoryBuffer memSizeBuf;
    pTile->Serialize(memSizeBuf, true, false, this);

    uint32 memSize = memSizeBuf.GetOffset();
    packet.extraDataSize = memSize;

    uint8* pTileData = new uint8[memSize];
    MemoryBuffer memBuffer(pTileData, memSize);
    pTile->Serialize(memBuffer, true, false, this);

    if(pPlayer)
    {
        SendENetPacketRaw(NET_MESSAGE_GAME_PACKET, &packet, sizeof(GameUpdatePacket), pTileData, pPlayer->GetPeer());
    }
    else
    {
        SendGamePacketToAll(&packet, nullptr, pTileData);
    }

    SAFE_DELETE_ARRAY(pTileData);
}

void World::SendTileUpdateMultiple(const std::vector<TileInfo*>& tiles)
{
    if(tiles.empty()) {
        return;
    }

    MemoryBuffer memSizeBuf;
    for(auto& pTile : tiles) {
        pTile->Serialize(memSizeBuf, true, false, this);
    }

    uint32 memSize = tiles.size() * 2 * sizeof(int32) + memSizeBuf.GetOffset() + sizeof(int32);
    uint8* pData = new uint8[memSize];

    MemoryBuffer memBuffer(pData, memSize);
    for(auto& pTile : tiles) {
        Vector2Int vTilePos = pTile->GetPos();
        memBuffer.Write((int32)vTilePos.x);
        memBuffer.Write((int32)vTilePos.y);

        pTile->Serialize(memBuffer, true, false, this);
    }

    int32 endMarker = -1;
    memBuffer.Write(endMarker);

    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_SEND_TILE_UPDATE_DATA_MULTIPLE;
    packet.flags |= GAME_PACKET_FLAG_EXTENDED_DATA;
    packet.extraDataSize = memSize;
    packet.field_11 = -1;
    packet.field_12 = -1;
    
    SendGamePacketToAll(&packet, nullptr, pData);
    SAFE_DELETE_ARRAY(pData);
}

void World::SendTileApplyDamage(TileInfo* pTile, int32 damage, int32 netID)
{
    if(!pTile)
        return;

    Vector2Int& vTilePos = pTile->GetPos(); 

    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_TILE_APPLY_DAMAGE;
    packet.field_11 = vTilePos.x;
    packet.field_12 = vTilePos.y;
    packet.field_7 = damage;
    packet.field_4 = netID;

    SendGamePacketToAll(&packet);
}

void World::SendLockPacketToAll(int32 userID, int32 lockID, std::vector<TileInfo*>& tiles, TileInfo* pLockTile)
{
    if(!pLockTile)
        return;

    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_SEND_LOCK;
    packet.flags |= GAME_PACKET_FLAG_EXTENDED_DATA;
    packet.field_11 = pLockTile->GetPos().x;
    packet.field_12 = pLockTile->GetPos().y;
    packet.field_7 = lockID;
    packet.field_4 = userID;

    HandleTilePackets(&packet);

    uint8* pData = nullptr;

    if(!tiles.empty()) {
        packet.field_5 = tiles.size();
        packet.extraDataSize = tiles.size() * sizeof(uint16);
    
        uint32 memSize = packet.extraDataSize;
        pData = new uint8[memSize];
    
        MemoryBuffer memBuffer(pData, memSize);
        uint32 worldWidth = GetTileManager()->GetSize().x;
    
        for(auto& pTile : tiles) {
            if(!pTile) {
                continue;
            }
    
            Vector2Int vTilePos = pTile->GetPos();
            uint16 index = vTilePos.x + vTilePos.y * worldWidth;
            memBuffer.Write(index);
        }
    }

    SendGamePacketToAll(&packet, nullptr, pData);
    SAFE_DELETE_ARRAY(pData);
}

void World::SendPlayerDataConfigToAll(GamePlayer* pPlayer)
{
    if(!pPlayer) {
        return;
    }

    Role* pRole = pPlayer->GetRole();
    if(!pRole) {
        return;
    }

    bool hasMState = pRole->HasPerm(ROLE_PERM_MSTATE);
    bool hasSmState = pRole->HasPerm(ROLE_PERM_SMSTATE);

    for(auto& pWorldPlayer : m_players) {
        if(pWorldPlayer) {
            pWorldPlayer->SendOnDataConfig(hasMState, hasSmState, pPlayer);
        }
    }
}

void World::SendParticleEffectToAll(eParticleEffect effectType, const Vector2Float& pos, int32 delayMs, float angle)
{
    for(auto& pWorldPlayer : m_players) {
        if(pWorldPlayer) {
            pWorldPlayer->SendOnParticleEffect(effectType, pos, delayMs, angle);
        }
    }
}

void World::SendHarvestTreeToAll(TileInfo* pTile, GamePlayer* pPlayer)
{
    if(!pTile)
        return;

    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_SEND_TILE_TREE_STATE;

    Vector2Int vTilePos = pTile->GetPos();
    packet.field_11 = vTilePos.x;
    packet.field_12 = vTilePos.y;

    if(pPlayer)
    {
        packet.field_4 = pPlayer->GetNetID();
    }
    else
    {
        packet.field_4 = -1;
    }

    packet.field_5 = -1;

    HandleTilePackets(&packet);
    SendGamePacketToAll(&packet);
}

void World::PlaySFXForEveryone(const string& fileName, int32 delay)
{
    for(auto& pWorldPlayer : m_players) {
        if(pWorldPlayer) {
            pWorldPlayer->PlaySFX(fileName, delay);
        }
    }
}

void World::SendPlayPositionedToAll(GamePlayer* pPlayer, const string& audio)
{
    for(auto& pWorldPlayer : m_players) {
        if(pWorldPlayer) {
            pWorldPlayer->SendOnPlayPositioned(audio, pPlayer);
        }
    }
}

void World::SendNPCPacketToAll(eNpcEvent eventType, uint8 npcID, uint8 npcType, const Vector2Float& pos, const Vector2Float& dest, float speed, int32 val1, int32 val2)
{
    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_NPC;
    packet.field_1 = npcType;
    packet.field_2 = npcID;
    packet.field_3 = eventType;
    packet.field_8 = pos;
    packet.field_9 = dest;
    packet.field_10 = speed;
    packet.field_11 = val1;
    packet.field_12 = val2;

    SendGamePacketToAll(&packet);
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

    switch(pGamePacket->type) 
    {
        case NET_GAME_PACKET_TILE_APPLY_DAMAGE:
        {
            TileInfo* pTile = GetTileManager()->GetTile(pGamePacket->field_11, pGamePacket->field_12);
            if(!pTile)
                return;

            pTile->PunchTile(pGamePacket->field_7);
            break;
        }

        case NET_GAME_PACKET_ITEM_CHANGE_OBJECT:
        {
            GetObjectManager()->HandleObjectPackets(pGamePacket);
            break;
        }

        case NET_GAME_PACKET_SEND_LOCK: 
        {
            TileInfo* pTile = GetTileManager()->GetTile(pGamePacket->field_11, pGamePacket->field_12);
            if(!pTile) {
                return;
            }

            pTile->SetFG(pGamePacket->field_7, GetTileManager());

            TileExtra_Lock* pTileExtra = pTile->GetExtra<TileExtra_Lock>();
            if(!pTileExtra) {
                return;
            }

            pTileExtra->ownerID = pGamePacket->field_4;
            SendTileUpdate(pGamePacket->field_11, pGamePacket->field_12);
            break;
        }

        case NET_GAME_PACKET_TILE_CHANGE_REQUEST: {
            TileInfo* pTile = GetTileManager()->GetTile(pGamePacket->field_11, pGamePacket->field_12);
            if(!pTile)
                return;
    
            ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pGamePacket->field_7);
            if(!pItem)
                return;

            if(pItem->IsBackground()) 
            {
                pTile->SetBG(pItem->id);
            }
            else if(pItem->type == ITEM_TYPE_FIST) 
            {
                if(pTile->GetFG() != ITEM_ID_BLANK) 
                {
                    pTile->SetFG(ITEM_ID_BLANK, GetTileManager());
                }
                else 
                {
                    pTile->SetBG(ITEM_ID_BLANK);
                }
            }
            else {
                if(pItem->IsBackground()) 
                {
                    pTile->SetBG(pItem->id);
                }
                else 
                {
                    pTile->SetFG(pItem->id, GetTileManager());
                }
            }

            if(pItem->IsBackground())
            {
                if(pItem->HasFlag(ITEM_FLAG_FLIPPABLE))
                {
                    if(pGamePacket->HasFlag(GAME_PACKET_FLAG_FACING_LEFT))
                    {
                        pTile->SetFlag(TILE_FLAG_FLIPPED_X);
                    }
                    else
                    {
                        pTile->RemoveFlag(TILE_FLAG_FLIPPED_X);
                    }
                }
            }
            else
            {
                if(pTile->IsTree())
                {
                    TileExtra_Seed* pTileExtra = pTile->GetExtra<TileExtra_Seed>();
                    if(pTileExtra)
                    {
                        if(pGamePacket->field_2 == 1)
                        {
                            pTile->SetFlag(TILE_FLAG_WILL_SPAWN_SEEDS_TOO);
                        }
                        else {
                            pTile->RemoveFlag(TILE_FLAG_WILL_SPAWN_SEEDS_TOO);
                        }
    
                        pTile->SetFlag(TILE_FLAG_IS_SEEDLING);
                        pTileExtra->fruitCount = pGamePacket->field_3;
                    }
                }

                if(pItem->HasFlag(ITEM_FLAG_FLIPPABLE))
                {
                    if(pGamePacket->HasFlag(GAME_PACKET_FLAG_FACING_LEFT))
                    {
                        pTile->SetFlag(TILE_FLAG_FLIPPED_X);
                    }
                    else
                    {
                        pTile->RemoveFlag(TILE_FLAG_FLIPPED_X);
                    }
                }
            }

            break;
        }

        case NET_GAME_PACKET_SEND_TILE_TREE_STATE:
        {
            TileInfo* pTile = GetTileManager()->GetTile(pGamePacket->field_11, pGamePacket->field_12);
            if(!pTile)
                return;

            if(pGamePacket->field_5 = -1)
            {
                pTile->RemoveFlag(TILE_FLAG_PAINTED_WHITE);
                pTile->SetFG(ITEM_ID_BLANK, GetTileManager());
                return;
            }

            TileExtra_Seed* pTileExtra = pTile->GetExtra<TileExtra_Seed>();
            if(!pTileExtra)
                return;

            pTileExtra->fruitCount = pGamePacket->field_7;
            if(pGamePacket->field_2 == 1)
            {
                pTile->SetFlag(TILE_FLAG_WILL_SPAWN_SEEDS_TOO);
            }
            else 
            {
                pTile->RemoveFlag(TILE_FLAG_WILL_SPAWN_SEEDS_TOO);
            }

            if(pGamePacket->field_3 == 1)
            {
                pTile->SetFlag(TILE_FLAG_IS_SEEDLING);
            }
            else 
            {
                pTile->RemoveFlag(TILE_FLAG_IS_SEEDLING);
            }

            pTileExtra->growTime = pGamePacket->field_4;
            break;
        }
    }
}

void World::ThrowItemToPlayerFromPosition(GamePlayer* pPlayer, const Vector2Float& pos, int32 itemID, int32 count)
{
    if(!pPlayer)
        return;

    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_ITEM_EFFECT;
    packet.field_8.x = pos.x;
    packet.field_8.y = pos.y;
    packet.field_5 = pPlayer->GetNetID();
    packet.field_3 = 5;
    packet.field_4 = 0;
    packet.field_11 = itemID;
    packet.field_12 = count;

    SendGamePacketToAll(&packet);
}

void World::ThrowItemToPositionFromPlayer(GamePlayer* pPlayer, const Vector2Float& pos, int32 itemID, int32 count)
{
    if(!pPlayer)
        return;

    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_ITEM_EFFECT;
    packet.field_8.x = pos.x;
    packet.field_8.y = pos.y;
    packet.field_5 = pPlayer->GetNetID();
    packet.field_3 = 4;
    packet.field_4 = 0;
    packet.field_11 = itemID;
    packet.field_12 = count;

    SendGamePacketToAll(&packet);
}

uint32 World::PathfindCalcDistance(TileInfo* pNode, TileInfo* pStart, TileInfo* pGoal)
{
    if(!pNode || !pStart || !pGoal)
        return 999999;

    Vector2Int vNodePos = pNode->GetPos();
    Vector2Int vStartPos = pStart->GetPos();
    Vector2Int vGoalPos = pGoal->GetPos();

    return Abs(vNodePos.x - vStartPos.x) + Abs(vNodePos.y - vStartPos.y) + Abs(vNodePos.x - vGoalPos.x) + Abs(vNodePos.y - vGoalPos.y);
}

int32 World::PathfindGetShortestOpenTile(TileInfo* pStart, TileInfo* pGoal, std::vector<TileInfo*>& openList)
{
    int32 bestIndex = -1;
    uint32 bestDist = 9999999;

    for(int32 i = 0; i < openList.size(); ++i)
    {
        TileInfo* pTile = openList[i];
        if(!pTile) 
            continue;

        int32 dist = PathfindCalcDistance(pTile, pStart, pGoal);
        if(bestIndex == -1 || dist < bestDist)
        {
            bestIndex = i;
            bestDist = dist;
        }
    }

    return bestIndex;
}

bool World::PathfindAddNeighborsToList(GamePlayer* pPlayer, TileInfo* pStart, TileInfo* pGoal, std::vector<TileInfo*>& openList)
{
    if(!pStart || !pGoal)
        return false;

    WorldTileManager* pTileMgr = GetTileManager();
    Vector2Int vStartPos = pStart->GetPos();

    for (int32 dy = -1; dy <= 1; ++dy)
    {
        for(int32 dx = -1; dx <= 1; ++dx)
        {
            if(dx == 0 && dy == 0)
                continue;

            if(dx != 0 && dy != 0)
                continue;

            TileInfo* pTile = pTileMgr->GetTile(vStartPos.x + dx, vStartPos.y + dy);
            if(!pTile)
                continue;

            if(IsTileCollidableForPlayer(pPlayer, pTile, true))
                continue;

            if(pTile == pGoal)
                return true;

            bool exists = false;
            for(auto& pOpen : openList)
            {
                if(pOpen == pTile)
                {
                    exists = true;
                    break;
                }
            }

            if(!exists)
            {
                openList.push_back(pTile);
            }
        }
    }

    return false;
}

bool World::CanPlayerTravelToTile(GamePlayer* pPlayer, TileInfo* pStart, TileInfo* pGoal)
{
    if(!pStart || !pGoal)
        return false;

    if(pStart == pGoal)
        return true;

    WorldTileManager* pTileMgr = GetTileManager();

    std::vector<TileInfo*> openList;
    openList.reserve(64);
    openList.push_back(pStart);

    const int32 maxIteration = 350; // add to config?

    for(int32 i = 0; i < maxIteration; ++i)
    {
        int32 bestIndex = PathfindGetShortestOpenTile(pStart, pGoal, openList);

        if(bestIndex < 0)
            return false;

        TileInfo* pCurrent = openList[bestIndex];
        openList.erase(openList.begin() + bestIndex);

        if(PathfindAddNeighborsToList(pPlayer, pCurrent, pGoal, openList))
            return true;
    }

    return false;
}

bool World::CanPlayerTravelStraight(GamePlayer* pPlayer, TileInfo* pStart, TileInfo* pGoal)
{
    if(!pPlayer || !pStart || !pGoal)
        return false;

    if(pStart == pGoal)
        return true;

    Vector2Float vStartPos = pStart->GetWorldPosCenter();
    Vector2Float vGoalPos = pGoal->GetWorldPosCenter();

    Vector2Float d = vGoalPos - vStartPos;
    float distance = Sqrt(d.x * d.x + d.y * d.y);

    int32 step = int32(distance / 32.0f) * 2 + 2;
    d /= step;

    TileInfo* pPrevTile = nullptr;
    
    float absDx = Abs(d.x);
    float absDy = Abs(d.y);

    WorldTileManager* pTileMgr = GetTileManager();

    if(Abs(absDx - absDy) >= 0.1)
    {
        for(int32 i = 0; i < step; ++i)
        {
            TileInfo* pTile = pTileMgr->GetTileByWorldPos(vStartPos);

            if(pTile != pPrevTile)
            {
                pPrevTile = pTile;

                if(pTile)
                {
                    if(IsTileCollidableForPlayer(pPlayer, pTile, true))
                        return false;
                }
            }

            vStartPos += d;
        }

        return true;
    }

    return CanPlayerTravelToTile(pPlayer, pStart, pGoal);
}

bool World::IsTileCollidableForPlayer(GamePlayer* pPlayer, TileInfo* pTile, bool ignorePlatforms)
{
    if(!pTile || pTile->IsCollidable())
        return true;

    uint16 displayedItem = pTile->GetDisplayedItem();
    if(displayedItem == 0)
        return false;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(displayedItem);
    if(!pItem)
        return true;

    if(pItem->collisionType == COLLISION_NONE)
        return false;

    if(ignorePlatforms && (pItem->collisionType == COLLISION_JUMP_THROUGH || pItem->collisionType == COLLISION_JUMP_DOWN))
        return false;

    if(pItem->collisionType == COLLISION_ONE_WAY)
        return false;

    if(pItem->collisionType == COLLISION_GATEWAY)
    {
        if(PlayerHasAccessOnTile(pPlayer, pTile) || pTile->HasFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC))
            return false;
    }

    if(pItem->collisionType == COLLISION_VIP)
    {
        /**
         * todo
         */

        return true;
    }

    if(pItem->collisionType == COLLISION_ADVENTURE)
    {
        /**
         * todo
         */

        return true;
    }

    if(pItem->collisionType == COLLISION_FACTION)
    {
        /**
         * todo but laaaaaaaaaaaaaaaaaaaaaaaaater
         */

        return true;
    }

    if(pItem->collisionType == COLLISION_CLOUD)
    {
        /**
         * todo
         */
    }

    if(pItem->collisionType == COLLISION_IF_OFF)
    {
        return !pTile->HasFlag(TILE_FLAG_IS_ON);
    }

    if(pItem->collisionType == COLLISION_IF_ON)
    {
        /**
         * todo
         */
    }

    if(pItem->collisionType == COLLISION_GUILD)
    {
        /**
         * todo
         */
    }

    return true;
}

bool World::PlayerHasAccessOnTile(GamePlayer* pPlayer, TileInfo* pTile)
{
    if(!pPlayer || !pTile) {
        return false;
    }

    if(pTile->HasFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC)) {
        return true;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetDisplayedItem());
    if(pItem->HasFlag(ITEM_FLAG_PUBLIC)) {
        return true;
    }

    TileInfo* pWorldLockTile = GetTileManager()->GetKeyTile(KEY_TILE_WORLD_LOCK);
    if(pWorldLockTile) {
        TileExtra_Lock* pWLExtra = pWorldLockTile->GetExtra<TileExtra_Lock>();
        if(!pWLExtra) {
            return false;
        }

        return pWLExtra->HasAccess(pPlayer->GetUserID());
    }

    if(pTile->GetParent() == 0) {
        return true;
    }

    TileInfo* pMainLockTile = GetTileManager()->GetTile(pTile->GetParent());
    if(!pMainLockTile) {
        return false;
    }

    TileExtra_Lock* pMainLockExtra = pMainLockTile->GetExtra<TileExtra_Lock>();
    if(!pMainLockExtra) {
        return false;
    }

    return pMainLockExtra->HasAccess(pPlayer->GetUserID());
}

std::vector<GamePlayer*> World::GetPlayersInWorldRect(const RectFloat& rect)
{
    std::vector<GamePlayer*> out;

    for(auto& pPlayer : m_players)
    {
        if(!pPlayer)
            continue;

        if(rect.Intersects(pPlayer->GetPlayerWorldRect()))
        {
            out.push_back(pPlayer);
        }
    }

    return out;
}

bool World::FlameUpTile(TileInfo* pTile)
{
    if(!pTile)
        return false;

    if(!pTile->IsFlammable())
        return false;

    TileInfo* pTopTile = GetTileManager()->GetTile(pTile->GetPos());
    if(pTopTile)
    {
        ItemInfo* pTopItem = GetItemInfoManager()->GetItemByID(pTopTile->GetDisplayedItem());
        if(!pTopItem)
            return false;

        if(pTopItem->type == ITEM_TYPE_CHECKPOINT || pTopItem->type == ITEM_TYPE_TEAM)
            return false;
    }

    pTile->SetFlag(TILE_FLAG_ON_FIRE);

    switch(pTile->GetFG())
    {
        case ITEM_ID_STONE_EGG:
            pTile->SetFG(ITEM_ID_HATCHING_STONE_EGG, GetTileManager());
            break;

        case ITEM_ID_GHOST_EGG:
            pTile->SetFG(ITEM_ID_HATCHING_GHOST_EGG, GetTileManager());
            break;
        
        case ITEM_ID_WATER_EGG:
            pTile->SetFG(ITEM_ID_HATCHING_WATER_EGG, GetTileManager());
            break;

        case ITEM_ID_VOID_EGG:
            pTile->SetFG(ITEM_ID_HATCHING_VOID_EGG, GetTileManager());
            break;

        case ITEM_ID_EASTER_EGG:
            pTile->SetFG(ITEM_ID_HATCHING_EASTER_EGG, GetTileManager());
            break;

        case ITEM_ID_HIGHLY_COMBUSTIBLE_BOX:
        {
            pTile->SetFG(ITEM_ID_COMBUSTED_BOX, GetTileManager());
            
            bool consumed = false;
            auto objsInRect = GetObjectManager()->GetObjectsInRect(pTile->GetRect());

            static const std::unordered_map<uint32, uint32> burnBoxReward =
            {
                { ITEM_ID_PICKAXE, ITEM_ID_FIRE_AX },
                { ITEM_ID_FIREFIGHTER_PANTS_YELLOW, ITEM_ID_FIREFIGHTER_PANTS_RED },
                { ITEM_ID_FIREFIGHTER_JACKET_YELLOW, ITEM_ID_FIREFIGHTER_JACKET_RED },
                { ITEM_ID_FIREFIGHTER_HELMET_YELLOW, ITEM_ID_FIREFIGHTER_HELMET_RED },
                { ITEM_ID_VAMPIRE_CAPE, ITEM_ID_FLAMING_CAPE },
                { ITEM_ID_PET_SLIME, ITEM_ID_STRAWBERRY_SLIME },
                { ITEM_ID_HAPPY_UNICORN_BLOCK, ITEM_ID_ANGRY_UNICORN_BLOCK },
                { ITEM_ID_BLADE_FRAGMENT, ITEM_ID_TEMPERED_STEEL_FRAGMENT },
                { ITEM_ID_AXE_FRAGMENT, ITEM_ID_TEMPERED_AXE_FRAGMENT },
                { ITEM_ID_PARTY_TUNES, ITEM_ID_TEMPERED_PARTY_TUNES },
                { ITEM_ID_LAZY_COBRA, ITEM_ID_ROASTED_COBRA },
                { ITEM_ID_HOWLING_WOLF_EMBLEM, ITEM_ID_CHARBROILED_WOLF_EMBLEM },
                { ITEM_ID_AQUA_CAVE_CRYSTAL, ITEM_ID_PURPLE_CAVE_CRYSTAL },
                { ITEM_ID_DEEP_IRON_ORE, ITEM_ID_DEEP_IRON_INGOT },
                { ITEM_ID_COW, ITEM_ID_BURNT_LEATHER },
                { ITEM_ID_BUFFALO, ITEM_ID_BURNT_LEATHER },
                { ITEM_ID_COW_CUBE, ITEM_ID_BURNT_LEATHER },
                { ITEM_ID_HIGHLY_COMBUSTIBLE_BOX, ITEM_ID_COMBUSTED_BOX },
                { ITEM_ID_STEEL_SPIKE, ITEM_ID_TEMPERED_STEEL_SPIKE },
                { ITEM_ID_SURGICAL_SCALPEL, ITEM_ID_LIMB_CONNECTOR },
                { ITEM_ID_SURGICAL_PINS, ITEM_ID_LIMB_CONNECTOR },
                { ITEM_ID_SURGICAL_CLAMP, ITEM_ID_LIMB_CONNECTOR },
                { ITEM_ID_STAR_IRON, ITEM_ID_STAR_IRON_INGOT },
                { ITEM_ID_CHOCOLATE_BLOCK, ITEM_ID_DARK_CHOCOLATE_BLOCK },
                { ITEM_ID_TINY_HORSIE, ITEM_ID_FLAMING_HORSE_CHUNK },
                { ITEM_ID_TINY_APPALOOSA, ITEM_ID_FLAMING_HORSE_CHUNK },
                { ITEM_ID_ICE_HORSE, ITEM_ID_FLAMING_HORSE_CHUNK },
                { ITEM_ID_IRISH_SPORT_HORSIE, ITEM_ID_FLAMING_HORSE_CHUNK },
                { ITEM_ID_WHITE_FURY, ITEM_ID_FLAMING_HORSE_CHUNK },
                { ITEM_ID_FLAMING_HORSIE, ITEM_ID_FLAMING_HORSE_CHUNK },
                { ITEM_ID_FLAME_TEE, ITEM_ID_ULTRA_FLAME_TEE },
                { ITEM_ID_AMBER_BLOCK, ITEM_ID_AMBER_RESIN },
                { ITEM_ID_DEAD_THANKSGIVING_TURKEY, ITEM_ID_ROASTED_TURKEY },
                { ITEM_ID_OBSIDIAN, ITEM_ID_DRAGON_GLASS },
                { ITEM_ID_MAGIC_ORE, ITEM_ID_MAGIC_INGOT },
            };

            for(auto& pObj : objsInRect)
            {
                if(!pObj)
                    continue;
 
                auto it = burnBoxReward.find(pObj->itemID);
                if(it == burnBoxReward.end())
                    continue;

                consumed = true;

                if(pObj->itemID == ITEM_ID_OBSIDIAN)
                {
                    if(pObj->count >= 20)
                    {
                        int32 remain = pObj->count % 20;
                        int32 cooked = pObj->count / 20;

                        pObj->itemID = it->second;
                        pObj->count = cooked;
                        ModifyObject(*pObj);

                        if(remain > 0)
                        {
                            DropObjectOnTile(pTile, ITEM_ID_OBSIDIAN, remain, GetRandomItemDropOffset(), true);
                        }

                    }
                }
                else
                {
                    pObj->itemID = it->second;
                    ModifyObject(*pObj);
                }
            }

            if(consumed)
            {
                SendParticleEffectToAll(PARTICLE_EFFECT_SMOKE, pTile->GetWorldPosCenter());
            }
        }
    }

    SendParticleEffectToAll(PARTICLE_EFFECT_FIRESTARTER, pTile->GetWorldPosCenter());
    return true;
}

void World::PutOutFire(TileInfo* pTile, GamePlayer* pPlayer)
{
    if(!pTile)
        return;

    pTile->RemoveFlag(TILE_FLAG_ON_FIRE);

    if(pPlayer)
    {
        PlayerProgress& progressData = pPlayer->GetProgressData();
        progressData.AddProgress(PLAYER_PROGRESS_PUT_OUT_FIRE_COUNT, 1);

        if(progressData.GetProgress(PLAYER_PROGRESS_PUT_OUT_FIRE_COUNT) % 100 == 0)
        {
            if(pPlayer->GetInventory().GetFitItemCount(ITEM_ID_HIGHLY_COMBUSTIBLE_BOX) != 0)
            {
                pPlayer->ModifyInventoryItem(ITEM_ID_HIGHLY_COMBUSTIBLE_BOX, 1);
                ThrowItemToPlayerFromPosition(pPlayer, pTile->GetWorldPosCenter(), ITEM_ID_HIGHLY_COMBUSTIBLE_BOX, 1);
            }
            else
            {
                DropObjectOnTile(pTile, ITEM_ID_HIGHLY_COMBUSTIBLE_BOX, 1, GetRandomItemDropOffset(), true);
            }

            pPlayer->SendOnTalkBubble("I'm so good at fighting fires, I rescued this `2Highly Combustible Box``!", false);
        }

        pPlayer->GiveXP(1);
    }

    SendParticleEffectToAll(PARTICLE_EFFECT_SIZZLE, pTile->GetWorldPosCenter());
}

void World::OnAddLock(GamePlayer* pPlayer, TileInfo* pTile, uint16 lockID)
{
    if(!pPlayer || !pTile) {
        return;
    }

    if(pPlayer->GetInventory().GetCountOfItem(lockID) == 0) {
        return;
    }

    if(pTile->GetFG() != ITEM_ID_BLANK) {
        pPlayer->SendOnTalkBubble("Use a lock on blank tile next to the things you want to lock.", true);
        return;
    }

    if(IsWorldLock(lockID) && GetTileManager()->GetKeyTile(KEY_TILE_WORLD_LOCK)) {
        pPlayer->SendOnTalkBubble("Only one `$World Lock`` can be placed in a world, you'd have to remove the other one first.", true);
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(lockID);
    if(!pItem) {
        return;
    }

    std::vector<TileInfo*> lockedTiles;

    if(!IsWorldLock(lockID)) {
        bool lockSuccsess = GetTileManager()->ApplyLockTiles(pTile, GetMaxTilesToLock(lockID), false, lockedTiles);
        if(!lockSuccsess) {
            pPlayer->SendOnTalkBubble("Something went wrong, unable to place lock in here", true);
            return;
        }
    }

    if(pTile->HasFlag(TILE_FLAG_PAINTED_RED | TILE_FLAG_PAINTED_GREEN | TILE_FLAG_PAINTED_BLUE)) 
    {
        pTile->RemoveFlag(TILE_FLAG_PAINTED_RED | TILE_FLAG_PAINTED_GREEN | TILE_FLAG_PAINTED_BLUE);
    }
    pTile->RemoveFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC);

    SendLockPacketToAll((int32)pPlayer->GetUserID(), (int32)lockID, lockedTiles, pTile);
    pPlayer->ModifyInventoryItem(lockID, -1);

    if(IsWorldLock(lockID)) {
        string playerName = pPlayer->GetDisplayName(false);

        SendTalkBubbleAndConsoleToAll("`5[`w" + GetWorlName() +  " has been `$World Locked`` by " + playerName + "`5]``", true, pPlayer);
        LOGGER_LOG_INFO("World %s has been locked by %d", GetWorlName().c_str(), pPlayer->GetUserID());

        SendNameChangeToAll(pPlayer);
    }
    else {
        pPlayer->SendOnTalkBubble("Area locked.", true);
        pPlayer->SendOnPlayPositioned("use_lock.wav");
    }

    pPlayer->GetProgressData().AddProgress(PLAYER_PROGRESS_PLACE_COUNT, 1);
}

void World::OnRemoveLock(GamePlayer* pPlayer, TileInfo* pTile)
{
    if(!pPlayer || !pTile) {
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if(!pItem || pItem->type != ITEM_TYPE_LOCK) {
        return;
    }

    if(IsWorldLock(pItem->id)) {
        SendConsoleMessageToAll("`5[```w" + GetWorlName() + "`` has had its `$World Lock`` removed!`5]``");
        LOGGER_LOG_INFO("Removed world lock in %s (%d) by %d", GetWorlName().c_str(), GetDatabaseID(), pPlayer->GetUserID());
    }
    else {
        std::vector<TileInfo*> unlockedTiles = GetTileManager()->RemoveTileParentsLockedBy(pTile);
        SendTileUpdateMultiple(unlockedTiles);
    }
}

void World::OnPunchedLock(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem)
{
    if(!pPlayer || !pTile || !pItem)
        return;

    if(pItem->type != ITEM_TYPE_LOCK)
        return;

    TileExtra_Lock* pTileExtra = pTile->GetExtra<TileExtra_Lock>();
    if(!pTileExtra)
        return;

    if(pTileExtra->ownerID == pPlayer->GetUserID())
        return;

    UserCacheManager* pCacheMgr = GetUserCacheManager();
    Vector2Int& vTilePos = pTile->GetPos();

    pCacheMgr->FetchMetadata(
        pPlayer->GetNetID(),
        CACHE_REQ_WORLD_LOCK_PUNCH,
        { pTileExtra->ownerID },
        { GetInstanceID(), vTilePos.x, vTilePos.y }
    );
}

void World::OnPunchedAchievementBlock(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem)
{
    if(!pPlayer || !pTile || !pItem)
        return;

    if(pItem->type != ITEM_TYPE_ACHIEVEMENT)
        return;

    TileExtra_Achievement* pTileExtra = pTile->GetExtra<TileExtra_Achievement>();
    if(!pTileExtra)
        return;

    UserCacheManager* pCacheMgr = GetUserCacheManager();
    Vector2Int& vTilePos = pTile->GetPos();

    pCacheMgr->FetchMetadata(
        pPlayer->GetNetID(),
        CACHE_REQ_ACHIEVEMENT_BLOCK_PUNCH,
        { pTileExtra->ownerID },
        { GetInstanceID(), vTilePos.x, vTilePos.y }
    );
}

void World::OnPlantSeed(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pSeed, GameUpdatePacket* pPacket)
{
    if(!pPlayer || !pTile || !pSeed || !pPacket)
        return;

    if(!(pTile->IsTree() || pTile->GetDisplayedItem() == ITEM_ID_BLANK))
        return;
    
    if(!GetTileManager()->CanPlantTreeHere(pTile))
    {
        pPlayer->SendOnTalkBubble("Can't plant here!", true);
        return;
    }

    if(pTile->GetDisplayedItem() == ITEM_ID_BLANK)
    {
        uint32 fruitCount = 0;
        bool dropSeed = false;

        GetTreeSpawnInfo(pSeed, fruitCount, dropSeed);
        pPacket->field_4 = pPlayer->GetNetID();
        pPacket->field_3 = fruitCount;
        pPacket->field_2 = dropSeed ? 1 : 0;

        pPlayer->GetInventory().RemoveItem(pSeed->id, 1);
        pTile->SetFG(pSeed->id, GetTileManager());

        if(pPacket->field_2 == 1)
        {
            pTile->SetFlag(TILE_FLAG_WILL_SPAWN_SEEDS_TOO);
        }
        else
        {
            pTile->RemoveFlag(TILE_FLAG_WILL_SPAWN_SEEDS_TOO);
        }

        pTile->SetFlag(TILE_FLAG_IS_SEEDLING);

        TileExtra_Seed* pTileExtra = pTile->GetExtra<TileExtra_Seed>();
        if(pTileExtra)
        {
            pTileExtra->fruitCount = pPacket->field_3;
        }

        SendGamePacketToAll(pPacket);
    }
    else
    {
        if(pTile->HasFlag(TILE_FLAG_WAS_SPLICED))
        {
            pPlayer->SendOnTalkBubble("It would be too dangerous to try to mix three seeds.", true);
            return;
        }

        if(pTile->GetGrowthPercent() >= 100.0f)
        {
            pPlayer->SendOnTalkBubble("This tree is already too big to splice another seed with.", true);
            return;
        }

        ItemInfo* pTileSeed = GetItemInfoManager()->GetItemByID(pTile->GetFG());
        if(!pTileSeed)
            return;

        ItemInfo* pMixItem = GetItemInfoManager()->GetSpliceInfo(pTile->GetFG(), pSeed->id);
        if(!pMixItem)
        {
            pPlayer->SendOnTalkBubble(
                "Hmm, it looks like `w" + pTileSeed->name + "`` and `w" + pSeed->name + "`` can't be spliced.", true
            );
            return;
        }

        pPlayer->ModifyInventoryItem(pSeed->id, -1);

        pPlayer->SendOnTalkBubble(
            "`w" + pTileSeed->name + "`` and `w" + pSeed->name + "`` have been spliced to make a `$" + pMixItem->name + "!", true
        );
        pPlayer->PlaySFX("success.wav");

        pTile->SetFG(pMixItem->id, GetTileManager());
        pTile->SetFlag(TILE_FLAG_WAS_SPLICED);

        uint32 fruitCount = 0;
        bool dropSeed = false;
        GetTreeSpawnInfo(pSeed, fruitCount, dropSeed);

        if(dropSeed)
        {
            pTile->SetFlag(TILE_FLAG_IS_SEEDLING);
        }
        else
        {
            pTile->RemoveFlag(TILE_FLAG_IS_SEEDLING);
        }

        TileExtra_Seed* pTileExtra = pTile->GetExtra<TileExtra_Seed>();
        if(pTileExtra)
        {
            pTileExtra->fruitCount = fruitCount;
            pTile->ModGrowth(0, 0);
        }

        SendTileUpdate(pTile);
        return;
    }

    HandleTilePackets(pPacket);
}

void World::OnHarvestTree(GamePlayer* pPlayer, TileInfo* pTile)
{
    if(!pPlayer || !pTile)
        return;

    TileExtra_Seed* pTileExtra = pTile->GetExtra<TileExtra_Seed>();
    if(!pTileExtra)
        return;

    ItemInfoManager* pItemMgr = GetItemInfoManager();
    ItemInfo* pItem = pItemMgr->GetItemByID(pTile->GetFG());
    if(!pItem || pItem->type != ITEM_TYPE_SEED)
        return;

    uint32 fgItem = pTile->GetFG();

    if(fgItem == ITEM_ID_LEGENDARY_WIZARD_SEED)
    {
        if(RandomRangeInt(0, 1))
        {
            pTile->SetFlag(TILE_FLAG_FLIPPED_X);
        }
        else {
            pTile->RemoveFlag(TILE_FLAG_FLIPPED_X);
        }

        pTile->SetFG(ITEM_ID_LEGENDARY_WIZARD, GetTileManager());
        SendTileUpdate(pTile);

        SendParticleEffectToAll(PARTICLE_EFFECT_ACHIEVE, pTile->GetWorldPos());
        PlaySFXForEveryone("achievement.wav");
        return;
    }

    ItemInfo* pItemFruit = GetItemInfoManager()->GetItemByID(pItem->id - 1);
    if(!pItemFruit)
        return;

    uint32 fruitCount = pTileExtra->fruitCount;

    PlayerInventory& inventory = pPlayer->GetInventory();
    if(inventory.IsWearingItem(ITEM_ID_HAND_SCYTHE) && pItem->rarity < 100)
    {
        if(RandomRangeInt(0, 100) < 70)
        {
            pPlayer->ModifyInventoryItem(ITEM_ID_HAND_SCYTHE, -1);
            pPlayer->SendOnConsoleMessage("Your Hand Scythe broke!");
        }

        fruitCount *= 2;
    }
    else if(
        pItem->rarity < 100 &&
        (inventory.IsWearingItem(ITEM_ID_HARVESTER) || inventory.IsWearingItem(ITEM_ID_HARVESTER_OF_SORROWS)) &&
        IsFuelPack(inventory.GetClothByPart(BODY_PART_BACK))
    ) {
        if(RandomRangeInt(0, 100) < 10)
        {
            pPlayer->ModifyInventoryItem(inventory.GetClothByPart(BODY_PART_BACK), -1);
            fruitCount *= 2;
        }
    }

    if(pItem->farmablity > 0)
    {
        for(uint8 i = 0; i < fruitCount; ++i)
        {
            DropObjectOnTile(pTile, pItem->id - 1, RandomRangeInt(1, pItemFruit->farmablity), GetRandomItemDropOffset(), true);
        }

        uint32 gemAmount = GetGemCountHarvestTree(pItem);
        if(gemAmount > 0)
        {
            DropGemsOnTile(pTile, gemAmount);
        }
    
        if(pTile->HasFlag(TILE_FLAG_WILL_SPAWN_SEEDS_TOO))
        {
            ItemInfo* pSeed = pItemMgr->GetItemByID(pItemMgr->GetBaseItemID(pItem->id));
            if(pSeed)
            {
                DropObjectOnTile(pTile, pSeed->id, 1, GetRandomItemDropOffset(), true);
                pPlayer->SendOnTalkBubble("A `w" + pSeed->name + "`` falls out!", true);
            }
        }
    }

    SendHarvestTreeToAll(pTile, pPlayer);
}

void World::OnCollectProvider(GamePlayer* pPlayer, TileInfo* pTile)
{
    if(!pPlayer || !pTile)
        return;

    int32 itemIDToDrop = 0;
    uint32 dropCount = 0;

    switch(pTile->GetFG())
    {
        case ITEM_ID_SCIENCE_STATION:
        {
            static WeightRand scienceChemWeight(
            {
                {ITEM_ID_CHEMICAL_G, 38},
                {ITEM_ID_CHEMICAL_R, 23},
                {ITEM_ID_CHEMICAL_P, 6},
                {ITEM_ID_CHEMICAL_B, 12},
                {ITEM_ID_CHEMICAL_Y, 16}
            });

            if(!scienceChemWeight.Roll(itemIDToDrop))
                return;

            dropCount = 1;
            break;
        }
    }

    if(itemIDToDrop < 0 || itemIDToDrop == 0 || dropCount == 0)
        return;

    if(itemIDToDrop == ITEM_ID_GEMS)
    {
        pPlayer->ModifyGems(dropCount, true);
    }
    else
    {
        DropObjectOnTile(pTile, itemIDToDrop, dropCount, GetRandomItemDropOffset(), true);
    }

    pPlayer->GetProgressData().AddProgress(PLAYER_PROGRESS_COLLECT_PROVIDER, 1);
    pPlayer->GiveXP(1);

    TileExtra_Provider* pTileExtra = pTile->GetExtra<TileExtra_Provider>();
    if(pTileExtra)
    {
        pTile->ModGrowth(0, 0);
        pTile->FinalizeGrowth(0);

        pTileExtra->growTime = 0; // leave it for now
    }

    SendTileUpdate(pTile);
}

void World::OnTileDestroyedDropObject(GamePlayer* pPlayer, TileInfo* pTile)
{
    if(!pPlayer || !pTile)
        return;

    if(pTile->GetDisplayedItem() == ITEM_ID_BLANK)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetDisplayedItem());
    if(!pItem)
        return;

    if(pItem->type == ITEM_TYPE_SEED)
    {
        OnHarvestTree(pPlayer, pTile);
        return;
    }

    PlayerInventory& inventory = pPlayer->GetInventory();
    if(inventory.IsWearingItem(ITEM_ID_COSMIC_CAPE) && RandomRangeInt(0, 800) == 0)
    {
        DropObjectOnTile(pTile, ITEM_ID_COMET_DUST, 1, GetRandomItemDropOffset(), true);
    }


    if(pItem->type == ITEM_TYPE_TREASURE)
    {
        return;
    }

    bool dropBlock = false;
    bool dropSeed = false;
    int32 dropGems = 0;
    GetBlockSpawnInfo(pItem, false, dropBlock, dropSeed, dropGems);

    if(dropBlock)
    {
        DropObjectOnTile(pTile, pItem->id, 1, GetRandomItemDropOffset(), true);
    }

    if(dropSeed)
    {
        DropObjectOnTile(pTile, pItem->id + 1, 1, GetRandomItemDropOffset(), true);
    }

    if(dropGems > 0)
    {
        DropGemsOnTile(pTile, dropGems);
    }
}

void World::OnConsumeConsumable(GamePlayer* pPlayer, GamePlayer* pTarget, TileInfo* pTile, ItemInfo* pItem)
{
    if(!pPlayer || !pItem)
        return;

    if(pItem->type != ITEM_TYPE_CONSUMABLE)
        return;

    if(TileInfo* pJammerTile = GetTileManager()->GetKeyTile(KEY_TILE_GUARD_PINEAPPLE))
    {
        if(pJammerTile->HasFlag(TILE_FLAG_IS_ON))
        {
            pPlayer->SendOnTalkBubble("Can't use consumabled here!", true);
            return;
        }
    }

    ConsumableInfo* pConsumableInfo = GetItemInfoManager()->GetConsumableInfo(pItem->id);
    if(!pConsumableInfo)
        return;

    if(pPlayer->GetInventory().GetCountOfItem(pItem->id) < 1)
        return;

    if((pConsumableInfo->HasFlag(CONSUMABLE_FLAG_NEED_TARGET) && !pTarget) ||
        (pConsumableInfo->HasFlag(CONSUMABLE_FLAG_SELF_ONLY) && pPlayer != pTarget) ||
        (pConsumableInfo->HasFlag(CONSUMABLE_FLAG_NEED_TILE) && !pTile)
    ) {
        if(!pConsumableInfo->HasFlag(CONSUMABLE_FLAG_NEED_TARGET) || pTarget)
        {
            if(pConsumableInfo->HasFlag(CONSUMABLE_FLAG_SELF_ONLY) && pPlayer != pTarget)
            {
                pPlayer->SendOnTalkBubble("You can only use that on yourself.", true);
            }
        }
        else
        {
            pPlayer->SendOnTalkBubble("Must be used on a person.", true);
        }

        return;
    }

    if(pPlayer == pTarget && pItem->HasFlag(ITEM_FLAG_NOSELF))
    {
        pPlayer->SendOnTalkBubble("Use that on somebody else!", true);
        return;
    }

    if(pConsumableInfo->requiredAmount > pPlayer->GetInventory().GetCountOfItem(pConsumableInfo->itemID) && !pTarget)
    {
        if(!pConsumableInfo->failMessage.empty())
        {
            pPlayer->SendOnTalkBubble(pConsumableInfo->failMessage, false);
        }
        return;
    }
    
    if(pItem->id == ITEM_ID_BALANCE_MOONCAKE)
    {
        pPlayer->SendOnTalkBubble("Ew, it has raisins! I'm not eating that.", false);
        return;
    }
    else if(pItem->id == ITEM_ID_ANCESTOR_MOONCAKE)
    {
        pPlayer->SendOnTalkBubble("That's like a hundred years old. No.", false);
        return;
    }

    if(pPlayer == pTarget)
    {

    }

    PlayerInventory& inventory = pPlayer->GetInventory();

    if(pConsumableInfo->consumableType == CONSUMABLE_TYPE_NONE)
    {
        switch(pConsumableInfo->itemID)
        {
            case ITEM_ID_POCKET_LIGHTER:
            {
                if(pTile->HasFlag(TILE_FLAG_ON_FIRE))
                {
                    pPlayer->SendOnTalkBubble("That block can't get any hotter!", false);
                    return;
                }

                if(pTile->GetDisplayedItem() == ITEM_ID_BLANK)
                {
                    pPlayer->SendOnTalkBubble("There's nothing to burn!", false);
                    return;
                }

                if(pTile->HasFlag(TILE_FLAG_IS_WET))
                {
                    pPlayer->SendOnTalkBubble("You can't burn water.", false);
                    return;
                }

                if(!pTile->IsFlammable())
                {
                    pPlayer->SendOnTalkBubble("That won't burn!", false);
                    return;
                }

                Vector2Int& vTilePos = pTile->GetPos();
                TileInfo* pTopTile = GetTileManager()->GetTile(vTilePos.x, vTilePos.y - 1);
                if(pTopTile)
                {
                    ItemInfo* pTopItem = GetItemInfoManager()->GetItemByID(pTopTile->GetDisplayedItem());
                    if(!pTopItem)
                        return;

                    if(pTopItem->type == ITEM_TYPE_CHECKPOINT || pTopItem->type == ITEM_TYPE_TEAM)
                    {
                        pPlayer->SendOnTalkBubble("That'd just be rude.", false);
                        return;
                    }
                }

                if(!FlameUpTile(pTile))
                    return;

                SendTileUpdate(pTile);
                SendTalkBubbleAndConsoleToAll("`4MWAHAHAHA!! FIRE FIRE FIRE``", false, pPlayer);
                break;
            }

            case ITEM_ID_WATER_BUCKET:
            {
                if(pTile->HasFlag(TILE_FLAG_ON_FIRE))
                {
                    PutOutFire(pTile, pPlayer);
                    SendTileUpdate(pTile);
                    pPlayer->ModifyInventoryItem(ITEM_ID_WATER_BUCKET, -1);

                    float randX = RandomRangeFloat(0, 32);
                    float randY = RandomRangeFloat(0, 32);

                    Vector2Float vWorldPos = pTile->GetWorldPos();
                    vWorldPos.x += randX;
                    vWorldPos.y += randY;

                    ThrowItemToPositionFromPlayer(pPlayer, vWorldPos, ITEM_ID_WATER_BUCKET, 1);
                    return;
                }
                
                if(!PlayerHasAccessOnTile(pPlayer, pTile))
                    return;

                ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetDisplayedItem());
                if(!pItem)
                    return;

                if(pItem->id == ITEM_ID_MAIN_DOOR || pItem->type == ITEM_TYPE_BEDROCK)
                    return;

                bool isWetBefore = pTile->HasFlag(TILE_FLAG_IS_WET);

                pTile->ToggleFlag(TILE_FLAG_IS_WET);
                SendTileUpdate(pTile);

                float randX = RandomRangeFloat(0, 32);
                float randY = RandomRangeFloat(0, 32);

                Vector2Float vWorldPos = pTile->GetWorldPos();
                vWorldPos.x += randX;
                vWorldPos.y += randY;

                ThrowItemToPositionFromPlayer(pPlayer, vWorldPos, ITEM_ID_WATER_BUCKET, 1);

                if(isWetBefore)
                {
                    if(RandomRangeInt(0, 100) < 75)
                    {
                        pPlayer->ModifyInventoryItem(ITEM_ID_WATER_BUCKET, 1);
                    }
                }
                else
                {
                    pPlayer->ModifyInventoryItem(ITEM_ID_WATER_BUCKET, -1);
                }

                return;
            }

            case ITEM_ID_GHOST_JAR:
            {
                Vector2Int& vTilePos = pTile->GetPos();
                if(!m_pNpcManager->SpawnGhostTrap(pPlayer, vTilePos.x, vTilePos.y))
                    return;

                break;
            }

            case ITEM_ID_GHOST_IN_A_JAR:
            case ITEM_ID_MIND_GHOST_IN_A_JAR:
            {
                TileInfo* pGhostJammer = GetTileManager()->GetKeyTile(KEY_TILE_GHOST_CHARM);
                if(pGhostJammer && pGhostJammer->HasFlag(TILE_FLAG_IS_ON))
                {
                    pPlayer->SendOnTalkBubble("Ghosts are too scared to appear in this world!", false);
                    return;
                }

                eNPCType ghostType = NPC_TYPE_GHOST;
                if(pConsumableInfo->itemID == ITEM_ID_MIND_GHOST_IN_A_JAR) ghostType = NPC_TYPE_MIND_CONTROL_GHOST;

                Vector2Int& vTilePos = pTile->GetPos();
                if(!m_pNpcManager->SpawnGhost(ghostType, vTilePos.x, vTilePos.y, false))
                {
                    pPlayer->SendOnTalkBubble("This world is haunted enough already!", false);
                    return;
                }

                SendPlayPositionedToAll(pPlayer, "punch_glass.wav");
                break;
            }
        }
    }
    else if(pConsumableInfo->consumableType == CONSUMABLE_TYPE_CRAFT)
    {
        if(pConsumableInfo->rewardItemID == ITEM_ID_URANIUM_GLOWING_LURE)
        {
            if(inventory.GetClothByPart(BODY_PART_HAT) != ITEM_ID_HAZMAT_HELMET)
            {
                pPlayer->SendOnTalkBubble("`4You need a Hazmat Helmet or you'll end up horribly deformed!``", true);
                return;
            }

            if(inventory.GetClothByPart(BODY_PART_SHIRT) != ITEM_ID_HAZMAT_SUIT)
            {
                pPlayer->SendOnTalkBubble("`4You need a Hazmat Suit or you'll end up horribly deformed!``", true);
                return;
            }

            if(inventory.GetClothByPart(BODY_PART_PANT) != ITEM_ID_HAZMAT_PANTS)
            {
                pPlayer->SendOnTalkBubble("`4You need a Hazmat Pants or you'll end up horribly deformed!``", true);
                return;
            }

            if(inventory.GetClothByPart(BODY_PART_SHOE) != ITEM_ID_HAZMAT_BOOTS)
            {
                pPlayer->SendOnTalkBubble("`4You need a Hazmat Boots or you'll end up horribly deformed!``", true);
                return;
            }
        }

        if(pConsumableInfo->rewardItemID == ITEM_ID_MEGA_PELLET_BAIT)
        {
            if(inventory.GetClothByPart(BODY_PART_HAT) != ITEM_ID_BUCKSKIN_HOOD)
            {
                pPlayer->SendOnTalkBubble("4You need to be wearing a Buckskin Hood or you'll end up frozen!`", true);
                return;
            }

            if(inventory.GetClothByPart(BODY_PART_SHIRT) != ITEM_ID_BUCKSKIN_JACKET)
            {
                pPlayer->SendOnTalkBubble("4You need to be wearing a Buckskin Jacket or you'll end up frozen!`", true);
                return;
            }

            if(inventory.GetClothByPart(BODY_PART_CHESTITEM) != ITEM_ID_WINTER_SCARF)
            {
                pPlayer->SendOnTalkBubble("`4You need to be wearing a Winter Scarf or you'll end up frozen!``", true);
                return;
            }

            if(inventory.GetClothByPart(BODY_PART_PANT) != ITEM_ID_BUCKSKIN_PANTS)
            {
                pPlayer->SendOnTalkBubble("4You need to be wearing a Buckskin Pants or you'll end up frozen!`", true);
                return;
            }
        }

        uint8 fitCount = inventory.GetFitItemCount(pConsumableInfo->rewardItemID);
        if(fitCount == 0)
        {
            pPlayer->SendOnTalkBubble("You need a free slot to consume this item.", false);
            return;
        }

        pPlayer->ModifyInventoryItem(pConsumableInfo->rewardItemID, pConsumableInfo->rewardCount);

        if(!pConsumableInfo->successMessage.empty())
        {
            pPlayer->SendOnTalkBubble(pConsumableInfo->successMessage, false);
        }

        SendParticleEffectToAll(PARTICLE_EFFECT_TRANSFORM_FX, pPlayer->GetWorldPos());

        if(pConsumableInfo->HasFlag(CONSUMABLE_FLAG_EQUIP))
        {
            ItemInfo* pRewardItem = GetItemInfoManager()->GetItemByID(pConsumableInfo->rewardItemID);
            if(pRewardItem)
            {
                if(inventory.GetClothByPart((eBodyPart)pRewardItem->bodyPart) != pRewardItem->id)
                {
                    pPlayer->ToggleCloth(pRewardItem->id);
                }
            }
        }
    }

    pPlayer->GiveXP(pConsumableInfo->rewardCount);
    pPlayer->ModifyInventoryItem(pItem->id, -pConsumableInfo->requiredAmount);

    if(pItem->rarity > 1 && pItem->rarity < 999 && pTile && !pTile->IsCollidable())
    {
        bool dropBlock = false;
        bool dropSeed = false;
        int32 dropGems = 0;
        GetBlockSpawnInfo(pItem, false, dropBlock, dropSeed, dropGems);
    
        if(dropBlock)
        {
            DropObjectOnTile(pTile, pItem->id, 1, GetRandomItemDropOffset(), true);
        }
    
        if(dropSeed)
        {
            DropObjectOnTile(pTile, pItem->id + 1, 1, GetRandomItemDropOffset(), true);
        }
    
        if(dropGems > 0)
        {
            DropGemsOnTile(pTile, dropGems);
        }
    }
}

bool World::IsPlayerWorldOwner(GamePlayer* pPlayer)
{
    if(!pPlayer) {
        return false;
    }

    TileInfo* pLockTile = GetTileManager()->GetKeyTile(KEY_TILE_WORLD_LOCK);
    if(!pLockTile) {
        return false;
    }

    TileExtra_Lock* pTileExtra = pLockTile->GetExtra<TileExtra_Lock>();
    if(!pTileExtra) {
        return false;
    }

    if(pTileExtra->ownerID == pPlayer->GetUserID()) {
        return true;
    }

    return false;
}

bool World::IsPlayerWorldAdmin(GamePlayer* pPlayer)
{
    if(!pPlayer) {
        return false;
    }

    TileInfo* pLockTile = GetTileManager()->GetKeyTile(KEY_TILE_WORLD_LOCK);
    if(!pLockTile) {
        return false;
    }

    TileExtra_Lock* pTileExtra = pLockTile->GetExtra<TileExtra_Lock>();
    if(!pTileExtra) {
        return false;
    }

    if(pTileExtra->IsAdmin(pPlayer->GetUserID())) {
        return true;
    }

    return false;
}

int32 World::GetWorldOwnerID()
{
    TileInfo* pTile = GetTileManager()->GetKeyTile(KEY_TILE_WORLD_LOCK);
    if(!pTile)
        return -1;

    TileExtra_Lock* pTileExtra = pTile->GetExtra<TileExtra_Lock>();
    if(!pTileExtra)
        return -1;

    return pTileExtra->ownerID;
}

void World::DropGemsOnTile(TileInfo* pTile, uint32 gemCount)
{
    if(!pTile || gemCount == 0)
        return;

    auto objsInRect = GetObjectManager()->GetObjectsInRectByItemID(pTile->GetRect(), ITEM_ID_GEMS);

    uint32 total = gemCount;

    for(auto& pObj : objsInRect)
    {
        if(pObj && pObj->itemID == ITEM_ID_GEMS) 
        {
            total += pObj->count;
        }
    }

    auto types = ParseGemIntoGemTypes(total);

    Vector2Int vTilePos = pTile->GetPos();
    Vector2Float vBasePos = Vector2Float(vTilePos.x, vTilePos.y) * 32;
    vBasePos += 8.0f;

    uint32 i = 0;

    for(auto& pObj : objsInRect)
    {
        if(!pObj || pObj->itemID != ITEM_ID_GEMS)
            continue;

        if(i < types.size())
        {
            pObj->count = GetGemAmountByGemType(types[i]);

            ModifyObject(*pObj);
            i++;
        }
        else
        {
            if(pObj->objectID != 0)
            {
                RemoveObject(pObj->objectID);
            }
        }
    }

    for (; i < types.size(); ++i)
    {
        DropObject(ITEM_ID_GEMS, GetGemAmountByGemType(types[i]), vBasePos + GetRandomItemDropOffset());
    }
}

void World::DropObjectOnTile(TileInfo* pTile, uint16 itemID, uint8 count, const Vector2Float& offset, bool merge)
{
    if(!pTile)
        return;

    Vector2Int vTilePos = pTile->GetPos();
    Vector2Float vBasePos = Vector2Float(vTilePos.x, vTilePos.y) * 32;
    vBasePos += 8.0f;

    WorldObject obj;
    obj.itemID = itemID;
    obj.count = count;
    obj.pos = vBasePos + offset;

    if(merge) 
    {
        auto objsInRect = GetObjectManager()->GetObjectsInRectByItemID(pTile->GetRect(), itemID);
    
        if(!objsInRect.empty()) 
        {
            WorldObject tempObj = obj;
            objsInRect.push_back(&tempObj);

            ItemInfo* pItem = GetItemInfoManager()->GetItemByID(obj.itemID);
            if(!pItem)
                return;

            WorldObject* pBaseObj = nullptr;
    
            for(auto& pObj : objsInRect) 
            {
                if(!pObj || pObj->count == 0 || pObj->HasFlag(OBJECT_FLAG_NO_STACK))
                    continue;
    
                if(!pBaseObj) 
                {
                    pBaseObj = pObj;
                    continue;
                }
    
                if(pBaseObj == pObj)
                    continue;
    
                if(pBaseObj->count >= pItem->maxCanHold) 
                {
                    pBaseObj = pObj;
                    continue;
                }
    
                uint16 space = pItem->maxCanHold - pBaseObj->count;
                uint16 transfer = Min(space, pObj->count);
        
                if(transfer > 0) 
                {
                    pBaseObj->count += transfer;
                    pBaseObj->SetFlag(OBJECT_FLAG_DIRTY);
    
                    pObj->count -= transfer;
                    pObj->SetFlag(OBJECT_FLAG_DIRTY);
                }
    
                if(pBaseObj->count >= pItem->maxCanHold) 
                {
                    pBaseObj = pObj;
                }
            }

            for(auto& pObj : objsInRect) 
            {
                if(!pObj) {
                    continue;
                }

                if(pObj->count == 0)
                {
                    if(pObj->objectID != 0)
                    {
                        RemoveObject(pObj->objectID);
                    }
            
                    continue;
                }
            
                if(pObj->objectID == 0 && pObj->count > 0)
                {
                    pObj->pos = vBasePos;
                    DropObject(*pObj);
                }
                else if(pObj->HasFlag(OBJECT_FLAG_DIRTY))
                {
                    pObj->RemoveFlag(OBJECT_FLAG_DIRTY);
                    ModifyObject(*pObj);
                }
            }
        }
        else {
            DropObject(obj);
        }

        return;
    }

    DropObject(obj);
}

void World::DropObject(uint16 itemID, uint8 count, const Vector2Float& pos)
{
    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_ITEM_CHANGE_OBJECT;
    packet.field_7 = itemID;
    packet.field_8.x = pos.x;
    packet.field_8.y = pos.y;
    packet.field_6 = count;
    //packet.worldObjectFlags = obj.flags;
    packet.field_4 = -1;

    GetObjectManager()->HandleObjectPackets(&packet);
    SendGamePacketToAll(&packet);
}

void World::DropObject(const WorldObject& obj)
{
    DropObject(obj.itemID, obj.count, obj.pos);
}

void World::RemoveObject(uint32 objectID)
{
    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_ITEM_CHANGE_OBJECT;
    packet.field_7 = objectID;
    packet.field_4 = -2;
    packet.field_5 = -1;

    GetObjectManager()->HandleObjectPackets(&packet);
    SendGamePacketToAll(&packet);
}

void World::ModifyObject(const WorldObject& obj)
{
    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_ITEM_CHANGE_OBJECT;
    packet.field_7 = obj.itemID;
    packet.field_8.x = obj.pos.x;
    packet.field_8.y = obj.pos.y;
    packet.field_6 = obj.count;
    packet.field_1 = obj.flags;
    packet.field_4 = -3;
    packet.field_5 = obj.objectID;

    GetObjectManager()->HandleObjectPackets(&packet);
    SendGamePacketToAll(&packet);
}
