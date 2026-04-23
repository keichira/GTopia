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

    Vector2Int GetSize() const { return m_size; }
    void SetSize(const Vector2Int& size) { m_size = size; }

    TileInfo* GetTile(int32 x, int32 y);
    TileInfo* GetKeyTile(eKeyTile keyTile);
    TileInfo* GetTile(int32 index);

    void ModifyKeyTile(TileInfo* pTile, bool remove);

    void GenerateDefaultMap();
    void GenerateClearMap();
    void GenerateBeachMap();

    void FillRectWith(const RectInt& rect, uint16 fgItem, uint16 bgItem, float chance);
    bool FillRectWith(const RectInt& rect, const TileMapFillVector& fgItems, const TileMapFillVector& bgItems);

    bool IsSameTile(TileInfo* pTile, int32 x, int32 y, bool forBackground);

    std::vector<TileInfo*> RemoveTileParentsLockedBy(TileInfo* pLockTile);
    bool AbleToLockThisTile(TileInfo* pLockTile, TileInfo* pTargetTile, bool ignoreEmpty);
    bool ApplyLockTiles(TileInfo* pLockTile, int32 tileSizeToLock, bool ignoreEmpty, std::vector<TileInfo*>& outTiles);

private:
    void FillRectWithThickness(uint16 thickness, RectInt& rect, uint16 fgItem, uint16 bgItem, float chance);
    void FillRectWithThickness(uint16 thickness, RectInt& rect, const TileMapFillVector& fgItems, const TileMapFillVector& bgItems);
    
private:
    Vector2Int m_size;
    std::vector<TileInfo> m_tiles;
    std::vector<TempTileData> m_tempTiles;

    std::vector<TileInfo*> m_keyTiles;
    std::vector<Vector2Int> m_onFireTiles;
};