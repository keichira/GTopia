#include "WorldTileManager.h"
#include "../Math/Math.h"
#include "../Item/ItemInfoManager.h"
#include "../Math/Random.h"

WorldTileManager::WorldTileManager()
: m_size(WORLD_DEFAULT_WIDTH, WORLD_DEFAULT_HEIGHT)
{
    m_keyTiles.reserve(KEY_TILE_SIZE);
}

bool WorldTileManager::Serialize(MemoryBuffer& memBuffer, bool write, bool database, uint16 worldVersion)
{
    memBuffer.ReadWrite(m_size, write);

    uint32 totalTiles = m_tiles.size();
    memBuffer.ReadWrite(totalTiles, write);

    if(totalTiles != (m_size.x * m_size.y) || totalTiles > (255*255)) {
        return false;
    }

    if(!write) {
        m_tiles.resize(totalTiles);
    }

    for(auto i = 0; i < m_tiles.size(); ++i) {
        m_tiles[i].Serialize(memBuffer, write, database, worldVersion);

        if(!write) {
            m_tiles[i].SetPos(i % m_size.x, i / m_size.x);
        }
    }

    return true;
}

void WorldTileManager::Clear(bool reInit)
{
    m_tiles.clear();
    m_keyTiles.clear();
    m_onFireTiles.clear();

    if(reInit) {
        m_tiles.resize(m_size.x * m_size.y);
        m_keyTiles.reserve(KEY_TILE_SIZE);
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

TileInfo* WorldTileManager::GetTile(eKeyTile keyTile)
{
    return m_keyTiles[keyTile];
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

#ifdef _DEBUG
    for(auto& tile : m_tiles) {
        if(tile.GetPos().y == layer.top - 1 || tile.GetPos().y == layer.top) {
            if(tile.GetPos().x < 10) {
                tile.SetFlag(TILE_FLAG_ON_FIRE);
            }
        }
    }
#endif

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

    bool startFromLeft = RandomRangeInt(0, 1) == 1;
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

void WorldTileManager::FillRectWithThickness(uint16 thickness, RectInt &rect, uint16 fgItem, uint16 bgItem, float chance)
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
