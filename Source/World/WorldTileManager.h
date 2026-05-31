#pragma once

#include "../Precompiled.h"
#include "TileInfo.h"
#include "../Memory/MemoryBuffer.h"
#include "../Math/Rect.h"

#define WORLD_DEFAULT_WIDTH 100
#define WORLD_DEFAULT_HEIGHT 60

enum eKeyTile
{
    KEY_TILE_MAIN_DOOR,
    KEY_TILE_GUARD_PINEAPPLE,
    KEY_TILE_WORLD_LOCK,
    KEY_TILE_PUNCH_JAMMER,
    KEY_TILE_ZOMBIE_JAMMER,
    KEY_TILE_SIGNAL_JAMMER,
    KEY_TILE_ANTIGRAVITY,
    KEY_TILE_GHOST_CHARM,

    KEY_TILE_SIZE
};

struct TileMapFillData
{
    uint32 itemID = 0;
    float chance = 100.0f;
};

typedef std::vector<TileMapFillData> TileMapFillVector;

class WorldInfo;

class WorldTileManager {
public:
    WorldTileManager();
    ~WorldTileManager();

public:
    bool Serialize(MemoryBuffer& memBuffer, bool write, bool database, WorldInfo* pWorld);
    uint32 GetMemEstimate(bool database, WorldInfo* pWorld);

    void Clear(bool reInit = false);

    Vector2Int& GetSize() { return m_size; }
    void SetSize(const Vector2Int& size) { m_size = size; }

    TileInfo* GetTile(const Vector2Int& pos);
    TileInfo* GetTile(int32 x, int32 y);
    TileInfo* GetKeyTile(eKeyTile keyTile);
    TileInfo* GetTile(int32 index);
    TileInfo* GetTileByWorldPos(float x, float y);
    TileInfo* GetTileByWorldPos(const Vector2Float& pos);

    int32 GetTileIndex(TileInfo* pTile);
    bool CanPlantTreeHere(TileInfo* pTile);

    void ModifyKeyTile(TileInfo* pTile, bool remove);

    void GenerateDefaultMap();
    void GenerateClearMap();
    void GenerateBeachMap();

    void FillRectWith(const RectInt& rect, uint16 fgItem, uint16 bgItem, float chance);
    bool FillRectWith(const RectInt& rect, const TileMapFillVector& fgItems, const TileMapFillVector& bgItems);

    bool IsSameTile(TileInfo* pTile, int32 x, int32 y, bool forBackground);
    TileInfo* GetTileByTypeFromRect(const RectFloat& rect, int32 itemType);

    std::vector<TileInfo*> RemoveTileParentsLockedBy(TileInfo* pLockTile);
    bool AbleToLockThisTile(TileInfo* pLockTile, TileInfo* pTargetTile, bool ignoreEmpty);
    bool ApplyLockTiles(TileInfo* pLockTile, int32 tileSizeToLock, bool ignoreEmpty, std::vector<TileInfo*>& outTiles);

    void AgeTiles(uint32 ageMS);
    bool RandomizeOuijaBoardTile(TileInfo* pTile);

    bool IsPowerNodeActiveInAGroup(TileInfo* pTile);
    void CheckPowerNodeToKill(TileInfo* pTile);
    void RebuildPowerNodeGroups();
    TileInfo* GetClosestPowerNodeFromWorldPos(const Vector2Float& pos);
    bool CheckIfPointInsidePowerNodeGroups(const Vector2Float& pos);

private:
    void FillRectWithThickness(uint16 thickness, RectInt& rect, uint16 fgItem, uint16 bgItem, float chance);
    void FillRectWithThickness(uint16 thickness, RectInt& rect, const TileMapFillVector& fgItems, const TileMapFillVector& bgItems);
    
private:
    Vector2Int m_size;
    std::vector<TileInfo> m_tiles;
    std::vector<TempTileData> m_tempTiles;

    std::vector<TileInfo*> m_keyTiles;
    std::vector<Vector2Int> m_onFireTiles;

    std::vector<TileInfo*> m_powerNodes;
    std::vector<std::vector<TileInfo*>>  m_powerNodeGroups;
    std::unordered_map<TileInfo*, uint32> m_nodeToGroup;
};