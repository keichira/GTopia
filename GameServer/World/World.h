#pragma once

#include "Precompiled.h"
#include "World/WorldInfo.h"
#include "Packet/GamePacket.h"
#include "Database/Table/WorldDBTable.h"
#include "../Player/GamePlayer.h"
#include "Item/ItemInfoManager.h"
#include <queue>

enum eWorldState
{
    WORLD_STATE_LOADING,
    WORLD_STATE_READY,
    WORLD_STATE_DELETE
};

class GamePlayer;

class World : public WorldInfo {
public:
    World();
    ~World();

public:
    void SetDatabaseID(uint32 id) { m_databaseID = id; }
    uint32 GetDatabaseID() const { return m_databaseID; }

    void SetState(eWorldState state) { m_state = state; }
    eWorldState GetState() const { return m_state; }

    void SetInstanceID(uint32 id) { m_instanceID = id; }
    uint32 GetInstanceID() const { return m_instanceID; }

    bool LoadFromFile();

    static void SaveToDatabaseCB(QueryTaskResult&& result);
    void SaveToDatabase();
    void Update();

    void QueuePendingPlayer(GamePlayer* pPlayer);
    bool HasPendingPlayers() const;
    GamePlayer* PopPendingPlayer();

    void AddPlayer(GamePlayer* pPlayer);
    void PlayerLeaveWorld(GamePlayer* pPlayer);
    void ReconnectPlayers();

    void SendSkinColorUpdateToAll(GamePlayer* pPlayer);
    void SendTalkBubbleAndConsoleToAll(const string& message, bool stackBubble, GamePlayer* pPlayer = nullptr);
    void SendConsoleMessageToAll(const string& message);
    void SendNameChangeToAll(GamePlayer* pPlayer);
    void SendSetCharPacketToAll(GamePlayer* pPlayer);
    void SendClothUpdateToAll(GamePlayer* pPlayer);
    void SendParticleEffectToAll(float coordX, float coordY, uint32 particleType, float particleSize = 0, int32 delay = -1);
    void SendTileUpdate(TileInfo* pTile);
    void SendTileUpdate(uint16 tileX, uint16 tileY);
    void SendTileUpdateMultiple(const std::vector<TileInfo*>& tiles);
    void SendTileApplyDamage(uint16 tileX, uint16 tileY, int32 damage, int32 netID);
    void SendLockPacketToAll(int32 userID, int32 lockID, std::vector<TileInfo*>& tiles, TileInfo* pLockTile);
    void SendPlayerDataConfigToAll(GamePlayer* pPlayer);
    void SendParticleEffectToAll(eParticleEffect effectType, const Vector2Float& pos, int32 delayMs = 0, float angle = 0.0f);
    void SendHarvestTreeToAll(TileInfo* pTile, GamePlayer* pPlayer);
    void PlaySFXForEveryone(const string& fileName, int32 delay = -1);

    void SendGamePacketToAll(GameUpdatePacket* pPacket, GamePlayer* pExceptMe = nullptr, uint8* pExtraData = nullptr);
    void HandleTilePackets(GameUpdatePacket* pGamePacket);

    uint32 PathfindCalcDistance(TileInfo* pNode, TileInfo* pStart, TileInfo* pGoal);
    int32 PathfindGetShortestOpenTile(TileInfo* pStart, TileInfo* pGoal, std::vector<TileInfo*>& openList);
    bool PathfindAddNeighborsToList(GamePlayer* pPlayer, TileInfo* pStart, TileInfo* pGoal, std::vector<TileInfo*>& openList);
    bool CanPlayerTravelToTile(GamePlayer* pPlayer, TileInfo* pStart, TileInfo* pGoal);

    bool IsTileCollidableForPlayer(GamePlayer* pPlayer, TileInfo* pTile, bool ignorePlatforms);
    bool PlayerHasAccessOnTile(GamePlayer* pPlayer, TileInfo* pTile);

    void OnAddLock(GamePlayer* pPlayer, TileInfo* pTile, uint16 lockID);
    void OnRemoveLock(GamePlayer* pPlayer, TileInfo* pTile);

    void OnPlantSeed(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pSeed, GameUpdatePacket* pPacket);
    void OnHarvestTree(GamePlayer* pPlayer, TileInfo* pTile);
    void OnCollectProvider(GamePlayer* pPlayer, TileInfo* pTile);

    bool IsPlayerWorldOwner(GamePlayer* pPlayer);
    bool IsPlayerWorldAdmin(GamePlayer* pPlayer);

    void DropGemsOnTile(TileInfo* pTile, uint32 gemCount);
    void DropObjectOnTile(TileInfo* pTile, uint16 itemID, uint8 count, const Vector2Float& offset, bool merge);
    void DropObject(uint16 itemID, uint8 count, const Vector2Float& pos);

    void SendCurrentWeatherToAll();
    uint32 GetPlayerCount() const { return m_players.size(); }
    Timer& GetLastSaveTime() { return m_worldLastSaveTime; }
    Timer& GetOfflineTime() { return m_worldOfflineTime; }

private:
    void DropObject(const WorldObject& obj);
    void RemoveObject(uint32 objectID);
    void ModifyObject(const WorldObject& obj);
    bool OnPlayerJoin(GamePlayer* pPlayer);

private:
    uint32 m_databaseID;
    uint32 m_instanceID;

    eWorldState m_state;

    std::queue<GamePlayer*> m_pendingPlayers;
    std::vector<GamePlayer*> m_players;

    Timer m_worldOfflineTime;
    Timer m_worldLastSaveTime;
};