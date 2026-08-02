#pragma once

#include "Item/ItemInfoManager.h"
#include "Item/ItemUtils.h"
#include "Math/Vector2.h"
#include "Precompiled.h"
#include "Renderer2D.h"
#include "World/WorldInfo.h"
#include <blend2d/blend2d.h>
#include <vector>

class WorldRenderer
{
public:
    WorldRenderer();
    ~WorldRenderer();

    bool LoadWorld(uint32 worldID, WorldInfo* pOutWorld);
    void Draw(WorldInfo* pWorld);
    void ComputeVisibleBG(WorldInfo* pWorld);

private:
    void DrawWeather(WorldInfo* pWorld);
    void DrawTile(WorldInfo* pWorld, TileInfo* pTile, int16 itemID);
    void DrawTileShadows(WorldInfo* pWorld);
    Vector2Int GetSpriteCoords(WorldInfo* pWorld, TileInfo* pTile, ItemInfo* pItem);
    bool IsFGTransparent(int16 itemID);

private:
    std::vector<uint8> m_visibleBG;
    uint32 m_cachedWidth;
    uint32 m_cachedHeight;
    Renderer2D m_renderer;
};