#include "WorldTileManager.h"
#include "../Math/Math.h"
#include "../Item/ItemInfoManager.h"
#include "../Math/Random.h"

WorldTileManager::WorldTileManager()
{
}

bool WorldTileManager::Serialize(MemoryBuffer& memBuffer, bool write, bool database)
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
        m_tiles[i].Serialize(memBuffer, write, database);

        if(!write) {
            m_tiles[i].SetPos(Vector2Int(i % m_size.x, i / m_size.x));
        }
    }

    return false;
}

void WorldTileManager::Clear(bool reInit)
{
    m_tiles.clear();

    if(reInit) {
        m_tiles.resize(m_size.x * m_size.y);
        for(auto i = 0; i < m_tiles.size(); ++i) { m_tiles[i].SetPos(Vector2Int(i % m_size.x, i / m_size.x));}
    }
}

void WorldTileManager::GenerateDefaultMap()
{
    Clear(true);

    RectInt layer(0, 0, m_size.x, 0);
    FillRectWithThickness(6, layer, ITEM_ID_BEDROCK, ITEM_ID_CAVE_BACKGROUND, 100);
    FillRectWithThickness(4, layer,
            { { ITEM_ID_DIRT, 60 }, { ITEM_ID_LAVA, 40 }, { ITEM_ID_ROCK, 2 } },
            { { ITEM_ID_CAVE_BACKGROUND, 100 } });
    FillRectWithThickness(29, layer,
            { { ITEM_ID_DIRT, 94 }, { ITEM_ID_ROCK, 6 } },
            { { ITEM_ID_CAVE_BACKGROUND, 100 } });    
    FillRectWithThickness(1, layer, ITEM_ID_DIRT, ITEM_ID_CAVE_BACKGROUND, 100);
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
    int32 yEnd = Min(m_size.x, Max(rect.top, rect.bottom));

    ItemInfoManager* pItemManager = GetItemInfoManager();
    ItemInfo* pItemFg = pItemManager->GetItemByID(fgItem);
    ItemInfo* pItemBg = pItemManager->GetItemByID(bgItem);

    float probablity = chance * 0.01f;

    for(auto y = yStart; y < yEnd; ++y) {
        TileInfo* pTile = &m_tiles[y * m_size.x + xStart];

        for(auto x = xStart; x < xEnd; ++x) {
            if(RandomNextFloat() <= probablity) {
                if(pItemFg) {
                    pTile->SetFG(pItemFg->id);
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

    auto pickItem = [&](const TileMapFillVector& items, float totalChance) -> ItemInfo*
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

            if(ItemInfo* pBgItem = pickItem(bgItems, totalBgChance)) {
                pTile->SetBG(pBgItem->id);
            }

            if(ItemInfo* pFgItem = pickItem(fgItems, totalFgChance)) {
                pTile->SetFG(pFgItem->id);
            }

            pTile++;
        }
    }

    return true;
}

void WorldTileManager::FillRectWithThickness(uint16 thickness, RectInt& rect, uint16 fgItem, uint16 bgItem, float chance)
{
    rect.bottom = rect.top;
    rect.top = rect.bottom + thickness;

    FillRectWith(rect, fgItem, bgItem, chance);
}

void WorldTileManager::FillRectWithThickness(uint16 thickness, RectInt& rect, const TileMapFillVector& fgItems, const TileMapFillVector& bgItems)
{
    rect.bottom = rect.top;
    rect.top = rect.bottom + thickness;

    FillRectWith(rect, fgItems, bgItems);
}
