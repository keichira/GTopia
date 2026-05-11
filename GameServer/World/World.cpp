#include "World.h"
#include "IO/Log.h"
#include "Packet/NetPacket.h"
#include "Item/ItemInfoManager.h"
#include "Math/Rect.h"
#include "Math/Math.h"
#include "../Context.h"
#include "IO/File.h"
#include "../Player/PlayerManager.h"
#include "WorldManager.h"
#include "../Server/MasterBroadway.h"

World::World()
: m_databaseID(0), m_state(WORLD_STATE_LOADING), m_instanceID(0)
{
}

World::~World()
{
}

bool World::LoadFromFile()
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

    if(!m_pendingPlayers.empty()) {
        uint32 count = 0;

        uint32 maxPlayerCount = GetContext()->GetGameConfig()->worldMaxPlayerCount;
        while(count != 8 && !m_pendingPlayers.empty()) 
        {
            GamePlayer* pPlayer = m_pendingPlayers.front();
            m_pendingPlayers.pop();

            if(m_players.size() + 1 >= maxPlayerCount) {
                pPlayer->SendOnConsoleMessage("Oops, `5" + GetWorlName() + "`` already has `4" + ToString(maxPlayerCount) + "`` people in it. Try again later.");
                pPlayer->SendOnFailedToEnterWorld();
                continue;
            }

            count++;
            OnPlayerJoin(pPlayer);
        }
    }

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

    if(m_worldLastSaveTime.GetElapsedTime() >= 40 * 60 * 1000) 
    {
        SaveToDatabase();
    }
}

bool World::OnPlayerJoin(GamePlayer* pPlayer)
{
    if(!pPlayer)
        return false;

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
    packet.netID = -1;
    packet.flags |= GAME_PACKET_FLAG_EXTENDED_DATA;
    packet.extraDataSize = worldMemSize;
    SendENetPacketRaw(NET_MESSAGE_GAME_PACKET, &packet, sizeof(GameUpdatePacket), pWorldData, pPlayer->GetPeer());
    SAFE_DELETE_ARRAY(pWorldData);

    pPlayer->SendOnSpawn(pPlayer->GetSpawnData(true));
    pPlayer->SendOnSetClothing();
    pPlayer->SendCharacterState();

    string playerDisplayName = pPlayer->GetDisplayName();
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

    return true;
}

void World::QueuePendingPlayer(GamePlayer* pPlayer)
{
    if(!pPlayer)
        return;

    m_pendingPlayers.push(pPlayer);
}

bool World::HasPendingPlayers() const
{
    return !m_pendingPlayers.empty();
}

GamePlayer* World::PopPendingPlayer()
{
    if(m_pendingPlayers.empty())
        return nullptr;

    GamePlayer* pPlayer = m_pendingPlayers.front();
    m_pendingPlayers.pop();
    return pPlayer;
}

void World::AddPlayer(GamePlayer* pPlayer)
{
    if(!pPlayer)
        return;

    if(m_state != WORLD_STATE_READY)
    {
        m_pendingPlayers.push(pPlayer);
        return;
    }

    pPlayer->SetJoiningWorld(true);
    m_pendingPlayers.push(pPlayer);
}

void World::PlayerLeaverWorld(GamePlayer *pPlayer)
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
    }

    if(m_players.empty()) 
    {
        m_worldOfflineTime.Reset();
    }
}

void World::SendSkinColorUpdateToAll(GamePlayer* pPlayer)
{
    if(!pPlayer)
        return;

    uint32 skinColor = pPlayer->GetCharData().GetSkinColor(true);
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

    string playerName = pPlayer->GetDisplayName();
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
    packet.posX = coordX;
    packet.posY = coordY;
    packet.particleEffectType = (float)particleType;
    packet.particleEffectSize = particleSize;
    packet.delay = delay;

    for(auto& pWorldPlayer : m_players) {
        SendGamePacketToAll(&packet);
    }
}

void World::SendTileUpdate(TileInfo* pTile)
{
    if(!pTile) {
        return;
    }

    Vector2Int vTilePos = pTile->GetPos();
    SendTileUpdate(vTilePos.x, vTilePos.y);
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
    packet.flags |= GAME_PACKET_FLAG_EXTENDED_DATA;

    MemoryBuffer memSizeBuf;
    pTile->Serialize(memSizeBuf, true, false, this);

    uint32 memSize = memSizeBuf.GetOffset();
    packet.extraDataSize = memSize;

    uint8* pTileData = new uint8[memSize];
    MemoryBuffer memBuffer(pTileData, memSize);
    pTile->Serialize(memBuffer, true, false, this);

    SendGamePacketToAll(&packet, nullptr, pTileData);

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
    packet.tileX = -1;
    packet.tileY = -1;
    
    SendGamePacketToAll(&packet, nullptr, pData);
    SAFE_DELETE_ARRAY(pData);
}

void World::SendTileApplyDamage(uint16 tileX, uint16 tileY, int32 damage, int32 netID)
{
    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_TILE_APPLY_DAMAGE;
    packet.tileX = tileX;
    packet.tileY = tileY;
    packet.tileDamage = damage;
    packet.netID = netID;

    SendGamePacketToAll(&packet);
}

void World::SendLockPacketToAll(int32 userID, int32 lockID, std::vector<TileInfo*>& tiles, TileInfo* pLockTile)
{
    if(!pLockTile) {
        return;
    }

    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_SEND_LOCK;
    packet.flags |= GAME_PACKET_FLAG_EXTENDED_DATA;
    packet.tileX = pLockTile->GetPos().x;
    packet.tileY = pLockTile->GetPos().y;
    packet.itemID = lockID;
    packet.userID = userID;

    HandleTilePackets(&packet);

    uint8* pData = nullptr;

    if(!tiles.empty()) {
        packet.tileCount = tiles.size();
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
        case NET_GAME_PACKET_SEND_LOCK: {
            TileInfo* pTile = GetTileManager()->GetTile(pGamePacket->tileX, pGamePacket->tileY);
            if(!pTile) {
                return;
            }

            pTile->SetFG(pGamePacket->itemID, GetTileManager());

            TileExtra_Lock* pTileExtra = pTile->GetExtra<TileExtra_Lock>();
            if(!pTileExtra) {
                return;
            }

            pTileExtra->ownerID = pGamePacket->userID;
            SendTileUpdate(pGamePacket->tileX, pGamePacket->tileY);
            break;
        }

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
                    if(pGamePacket->HasFlag(GAME_PACKET_FLAG_FACING_LEFT)) {
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
                    if(pGamePacket->HasFlag(GAME_PACKET_FLAG_FACING_LEFT)) {
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
        string playerName = pPlayer->GetDisplayName();

        SendTalkBubbleAndConsoleToAll("`5[`w" + GetWorlName() +  " has been `$World Locked`` by " + playerName + "`5]``", true, pPlayer);
        LOGGER_LOG_INFO("World %s has been locked by %d", GetWorlName().c_str(), pPlayer->GetUserID());

        SendNameChangeToAll(pPlayer);
    }
    else {
        pPlayer->SendOnTalkBubble("Area locked.", true);
        pPlayer->SendOnPlayPositioned("use_lock.wav");
    }
}

void World::OnRemoveLock(GamePlayer* pPlayer, TileInfo* pTile)
{
    if(!pPlayer || !pTile) {
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetDisplayedItem());
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
    packet.itemID = itemID;
    packet.posX = pos.x;
    packet.posY = pos.y;
    packet.worldObjectCount = count;
    //packet.worldObjectFlags = obj.flags;
    packet.worldObjectType = -1;

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
    packet.worldObjectID = objectID;
    packet.worldObjectType = -2;
    packet.field4 = -1;

    GetObjectManager()->HandleObjectPackets(&packet);
    SendGamePacketToAll(&packet);
}

void World::ModifyObject(const WorldObject& obj)
{
    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_ITEM_CHANGE_OBJECT;
    packet.itemID = obj.itemID;
    packet.posX = obj.pos.x;
    packet.posY = obj.pos.y;
    packet.worldObjectCount = obj.count;
    packet.worldObjectFlags = obj.flags;
    packet.worldObjectType = -3;
    packet.field4 = obj.objectID;

    GetObjectManager()->HandleObjectPackets(&packet);
    SendGamePacketToAll(&packet);
}
