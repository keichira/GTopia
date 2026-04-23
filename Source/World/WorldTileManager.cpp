#include "WorldTileManager.h"
#include "../Math/Math.h"
#include "../Item/ItemInfoManager.h"
#include "../Math/Random.h"
#include "WorldInfo.h"

WorldTileManager::WorldTileManager()
: m_size(WORLD_DEFAULT_WIDTH, WORLD_DEFAULT_HEIGHT)
{
    m_keyTiles.resize(KEY_TILE_SIZE, nullptr);
    m_tempTiles.resize(m_size.x * m_size.y);
}

WorldTileManager::~WorldTileManager()
{
    Clear();
}

bool WorldTileManager::Serialize(MemoryBuffer& memBuffer, bool write, bool database, WorldInfo* pWorld)
{
    memBuffer.ReadWrite(m_size, write);

    uint32 totalTiles = m_tiles.size();
    memBuffer.ReadWrite(totalTiles, write);

    if(totalTiles != (m_size.x * m_size.y) || totalTiles > (255*255)) {
        return false;
    }

    if(!write) {
        m_tiles.resize(totalTiles);
        m_tempTiles.resize(totalTiles);
    }

    if(write) {
        uint32 batchStartIdx = 0;

        for(uint32 i = 0; i < m_tiles.size(); ++i) {
            TileInfo* pTile = &m_tiles[i];

            if(pTile->HasFlag(TILE_FLAG_HAS_EXTRA_DATA) || pTile->HasFlag(TILE_FLAG_HAS_PARENT)) {
                uint32 batchCount = i - batchStartIdx;

                if(batchCount > 0) {
                    memBuffer.WriteRaw(m_tempTiles.data() + batchStartIdx, sizeof(TempTileData) * batchCount);
                }

                pTile->Serialize(memBuffer, write, database, pWorld);
                batchStartIdx = i + 1;
            }
            else {
                pTile->CopyTempData(&m_tempTiles[i]);
            }
        }

        uint32 remaining = m_tempTiles.size() - batchStartIdx;
        if(remaining > 0) {
            memBuffer.WriteRaw(m_tempTiles.data() + batchStartIdx, sizeof(TempTileData) * remaining);
        }

        return true;
    }

    for(auto i = 0; i < m_tiles.size(); ++i) {
        m_tiles[i].Serialize(memBuffer, write, database, pWorld);
        ModifyKeyTile(&m_tiles[i], false);

        if(!write) {
            m_tiles[i].SetPos(i % m_size.x, i / m_size.x);
        }
    }

    return true;
}

uint32 WorldTileManager::GetMemEstimate(bool database, WorldInfo* pWorld)
{
    MemoryBuffer memSize;

    uint32 headerSize = sizeof(m_size) + 4;
    memSize.WriteRaw(nullptr, headerSize);

    for(auto& tile : m_tiles) {
        if(tile.HasFlag(TILE_FLAG_HAS_EXTRA_DATA)) {
            tile.Serialize(memSize, true, database, pWorld);
        }
        else {
            uint32 size = 8;
            if(tile.HasFlag(TILE_FLAG_HAS_PARENT)) size += 2;
            memSize.WriteRaw(nullptr, size);
        }
    }

    return memSize.GetOffset();
}

void WorldTileManager::Clear(bool reInit)
{
    m_tiles.clear();
    m_keyTiles.clear();
    m_onFireTiles.clear();
    m_tempTiles.clear();

    if(reInit) {
        m_tiles.resize(m_size.x * m_size.y);
        m_tempTiles.resize(m_size.x * m_size.y);
        m_keyTiles.resize(KEY_TILE_SIZE, nullptr);
        for(uint32 i = 0; i < m_tiles.size(); ++i) { m_tiles[i].SetPos(i % m_size.x, i / m_size.x); }
    }
}

TileInfo* WorldTileManager::GetTile(int32 x, int32 y)
{
    if(x < 0 || y < 0 || x >= m_size.x || y >= m_size.y) {
        return nullptr;
    }

    return &m_tiles[y * m_size.x + x];
}

TileInfo* WorldTileManager::GetKeyTile(eKeyTile keyTile)
{
    return m_keyTiles[keyTile];
}

TileInfo* WorldTileManager::GetTile(int32 index)
{
    if(index < 0 || index > (m_size.x * m_size.y)) {
        return nullptr;
    }

    return &m_tiles[index];
}

void WorldTileManager::ModifyKeyTile(TileInfo* pTile, bool remove)
{
    if(IsMainDoor(pTile->GetFG())) {
        m_keyTiles[KEY_TILE_MAIN_DOOR] = remove ? nullptr : pTile;
    }
    else if(pTile->GetFG() == ITEM_ID_GUARDIAN_PINEAPPLE) {
        m_keyTiles[KEY_TILE_GUARD_PINEAPPLE] = remove ? nullptr : pTile;
    }
    else if(IsWorldLock(pTile->GetFG())) {
        m_keyTiles[KEY_TILE_WORLD_LOCK] = remove ? nullptr : pTile;
    }
}

void WorldTileManager::GenerateDefaultMap()
{
    Clear(true);

    RectInt layer(0, 0, m_size.x, m_size.y);
    FillRectWithThickness(6, layer, ITEM_ID_BEDROCK, ITEM_ID_CAVE_BACKGROUND, 100);  
    FillRectWithThickness(4, layer,
            { { ITEM_ID_DIRT, 60 }, { ITEM_ID_LAVA, 40 }, { ITEM_ID_ROCK, 2 } },
            { { ITEM_ID_CAVE_BACKGROUND, 100 } });
    FillRectWithThickness(25, layer,
            { { ITEM_ID_DIRT, 94 }, { ITEM_ID_ROCK, 6 } },
            { { ITEM_ID_CAVE_BACKGROUND, 100 } }); 
    FillRectWithThickness(1, layer, ITEM_ID_DIRT, ITEM_ID_CAVE_BACKGROUND, 100); 

    int32 doorPosX = RandomRangeInt(10, layer.Width() - 10);
    TileInfo* pDoorTile = GetTile(doorPosX, layer.top - 1);

    pDoorTile->SetFG(ITEM_ID_MAIN_DOOR, this);
    if(TileExtra_Door* pTileExtra = pDoorTile->GetExtra<TileExtra_Door>()) {
        pTileExtra->name = "EXIT";
    }

    TileInfo* pBedrockTile = GetTile(doorPosX, layer.top);
    pBedrockTile->SetFG(ITEM_ID_BEDROCK, this);
}

void WorldTileManager::GenerateClearMap()
{
    Clear(true);

    RectInt layer(0, 0, m_size.x, m_size.y);
    FillRectWithThickness(6, layer, ITEM_ID_BEDROCK, ITEM_ID_CAVE_BACKGROUND, 100);  

    bool mainDoorAtRight = RandomRangeInt(0, 1) == 1;
    
    TileInfo* pDoorTile = GetTile( mainDoorAtRight ? m_size.x : 0, layer.top - 1 );
    pDoorTile->SetFG(ITEM_ID_MAIN_DOOR, this);
    
    if(TileExtra_Door* pTileExtra = pDoorTile->GetExtra<TileExtra_Door>()) {
        pTileExtra->name = "EXIT";
    }
}

void WorldTileManager::GenerateBeachMap()
{
    Clear(true);

    bool startFromLeft = RandomRangeInt(0, 2) == 1;
}

void WorldTileManager::FillRectWith(const RectInt& rect, uint16 fgItem, uint16 bgItem, float chance)
{
    if(chance > 100.0f) {
        chance = 100.0f;
    }
    if(chance <= 0.0f) {
        return;
    }

    int32 xStart = Max(0, Min(rect.left, rect.right));
    int32 xEnd = Min(m_size.x, Max(rect.left, rect.right));
    int32 yStart = Max(0, Min(rect.top, rect.bottom));
    int32 yEnd = Min(m_size.y, Max(rect.top, rect.bottom));

    ItemInfoManager* pItemManager = GetItemInfoManager();
    ItemInfo* pItemFg = pItemManager->GetItemByID(fgItem);
    ItemInfo* pItemBg = pItemManager->GetItemByID(bgItem);

    float probablity = chance * 0.01f;

    for(auto y = yStart; y < yEnd; ++y) {
        TileInfo* pTile = &m_tiles[y * m_size.x + xStart];

        for(auto x = xStart; x < xEnd; ++x) {
            if(RandomNextFloat() <= probablity) {
                if(pItemFg) {
                    pTile->SetFG(pItemFg->id, this);
                }
                
                if(pItemBg) {
                    pTile->SetBG(pItemBg->id);
                }
            }

            pTile++;
        }
    }
}

bool WorldTileManager::FillRectWith(const RectInt& rect, const TileMapFillVector& fgItems, const TileMapFillVector& bgItems)
{
    if((fgItems.empty() && bgItems.empty())) {
        return false;
    }

    int32 xStart = Max(0, Min(rect.left, rect.right));
    int32 xEnd = Min(m_size.x, Max(rect.left, rect.right));
    int32 yStart = Max(0, Min(rect.top, rect.bottom));
    int32 yEnd = Min(m_size.x, Max(rect.top, rect.bottom));

    float totalFgChance = 0.0f;
    for(const auto& item : fgItems) {
        totalFgChance += Max(0.0f, Min(100.0f, item.chance));
    }

    float totalBgChance = 0.0f;
    for(const auto& item : bgItems) {
        totalBgChance += Max(0.0f, Min(100.0f, item.chance));
    }

    ItemInfoManager* pItemManager = GetItemInfoManager();

    auto PickItem = [&](const TileMapFillVector& items, float totalChance) -> ItemInfo*
    {
        if (totalChance <= 0.f)
            return nullptr;

        float roll = RandomNextFloat() * totalChance;
        float chance = 0.f;

        for (const auto& entry : items)
        {
            chance += Max(0.0f, Min(100.0f, entry.chance));
            if (roll <= chance)
                return pItemManager->GetItemByID(entry.itemID);
        }

        return nullptr;
    };

    for(auto y = yStart; y < yEnd; ++y) {
        TileInfo* pTile = &m_tiles[y * m_size.x + xStart];

        for(auto x = xStart; x < xEnd; ++x) {

            if(ItemInfo* pBgItem = PickItem(bgItems, totalBgChance)) {
                pTile->SetBG(pBgItem->id);
            }

            if(ItemInfo* pFgItem = PickItem(fgItems, totalFgChance)) {
                pTile->SetFG(pFgItem->id, this);
            }

            pTile++;
        }
    }

    return true;
}

bool WorldTileManager::IsSameTile(TileInfo* pTile, int32 x, int32 y, bool forBackground)
{
    TileInfo* pTarget = GetTile(x, y);
    if(!pTarget) {
        return false;
    }

    if(forBackground) {
        return pTile->GetBG() == pTarget->GetBG();
    }

    return pTile->GetFG() == pTarget->GetFG();
}

std::vector<TileInfo*> WorldTileManager::RemoveTileParentsLockedBy(TileInfo* pLockTile)
{
    std::vector<TileInfo*> unlockedTiles;

    if(!pLockTile) {
        return unlockedTiles;
    }

    Vector2Int vLockPos = pLockTile->GetPos();
    uint32 index = vLockPos.x + m_size.x * vLockPos.y;

    for(auto& tile : m_tiles) {
        if(tile.GetParent() == index) {
            tile.SetParent(0);
            tile.RemoveFlag(TILE_FLAG_HAS_PARENT);

            unlockedTiles.push_back(&tile);
        }
    }

    return unlockedTiles;
}

bool WorldTileManager::AbleToLockThisTile(TileInfo* pLockTile, TileInfo* pTargetTile, bool ignoreEmpty)
{
    if(!pLockTile || !pTargetTile || pLockTile == pTargetTile) {
        return false;
    }

    if(pTargetTile->HasFlag(TILE_FLAG_HAS_PARENT) || pTargetTile->GetParent() != 0) {
        return false;
    }

    if(ignoreEmpty && pTargetTile->GetDisplayedItem() == ITEM_ID_BLANK) {
        return false;
    }   

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTargetTile->GetDisplayedItem());
    if(!pItem) {
        return false;
    }

    if(pItem->type == ITEM_TYPE_LOCK || pItem->type == ITEM_TYPE_DOOR || pItem->type == ITEM_TYPE_BEDROCK) {
        return false;
    }

    int32 neighbors[4][2] = 
    {
        {1, 0}, {0, 1}, {-1, 0}, {0, -1}
    };
    Vector2Int vTargetPos = pTargetTile->GetPos();
    Vector2Int vLockPos = pLockTile->GetPos();

    uint32 parentIndex = vLockPos.x + m_size.x * vLockPos.y;

    for(int8 i = 0; i < 4; ++i) {
        TileInfo* pNeighbor = GetTile(vTargetPos.x + neighbors[i][0], vTargetPos.y + neighbors[i][1]);
        if(!pNeighbor) {
            continue;
        }

        if(
            pNeighbor->GetParent() == parentIndex || pNeighbor == pLockTile ||
            (vTargetPos.x + neighbors[i][0] == vLockPos.x + neighbors[i][0] && vTargetPos.y + neighbors[i][0] == vLockPos.y + neighbors[i][0])
        ) {
            return true;
        }
    }

    return false;
}

bool WorldTileManager::ApplyLockTiles(TileInfo* pLockTile, int32 tileSizeToLock, bool ignoreEmpty, std::vector<TileInfo*>& outTiles)
{
    if(!pLockTile || tileSizeToLock > (m_size.x * m_size.y) || tileSizeToLock == 0) {
        return false;
    }

    RemoveTileParentsLockedBy(pLockTile);

    Vector2Int vStartPos = pLockTile->GetPos();
    std::vector<TileInfo*> lockedTiles;

    int32 radius = 1;
    int32 totalLocked = 0;
    uint32 maxRadius = Max(m_size.x, m_size.y);

    while(totalLocked < tileSizeToLock && radius <= maxRadius) { // :/ is it really ok? is that O(n^3)???
        int32 minDistance = 111111111;
        TileInfo* pClosestTile = nullptr;
        
        for(int32 x = vStartPos.x - radius; x <= vStartPos.x + radius; ++x) {
            for(int32 y = vStartPos.y - radius; y <= vStartPos.y + radius; ++y) {
                TileInfo* pCurrTile = GetTile(x, y);
                if(!pCurrTile || pCurrTile == pLockTile) {
                    continue;
                }

                if(!AbleToLockThisTile(pLockTile, pCurrTile, ignoreEmpty)) {
                    continue;
                }

                int32 distance = Abs(x - vStartPos.x) + Abs(y - vStartPos.y);
                if(!pClosestTile || distance < minDistance) {
                    pClosestTile = pCurrTile;
                    minDistance = distance;
                }
            }
        }

        if(!pClosestTile) {
            radius++;
            continue;
        }

        pClosestTile->SetParent(vStartPos.x + m_size.x * vStartPos.y);
        pClosestTile->SetFlag(TILE_FLAG_HAS_PARENT);
        lockedTiles.push_back(pClosestTile);
        totalLocked++;
    }

    outTiles = std::move(lockedTiles);
    return true;
}

void WorldTileManager::FillRectWithThickness(uint16 thickness, RectInt& rect, uint16 fgItem, uint16 bgItem, float chance)
{
    rect.top = rect.bottom - thickness;
    FillRectWith(rect, fgItem, bgItem, chance);
    rect.bottom = rect.top;
}

void WorldTileManager::FillRectWithThickness(uint16 thickness, RectInt& rect, const TileMapFillVector& fgItems, const TileMapFillVector& bgItems)
{
    rect.top = rect.bottom - thickness;
    FillRectWith(rect, fgItems, bgItems);
    rect.bottom = rect.top;
}
