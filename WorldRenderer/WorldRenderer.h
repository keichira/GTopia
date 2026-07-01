#pragma once

#include "Item/ItemInfoManager.h"
#include "Item/ItemUtils.h"
#include "Math/Vector2.h"
#include "Precompiled.h"
#include "Renderer2D.h"
#include "World/WorldInfo.h"
#include <blend2d/blend2d.h>

class WorldRenderer
{
public:
    WorldRenderer();
    ~WorldRenderer();

public:
    bool LoadWorld(uint32 worldID);
    void Draw();
    void ComputeVisibleBG();

private:
    void DrawWeather();
    void DrawTile(TileInfo* pTile, int16 itemID);
    Vector2Int GetSpriteCoords(TileInfo* pTile, ItemInfo* pItem);

    bool IsFGTransparent(int16 itemID);

private:
    WorldInfo* m_pWorld;
    std::vector<uint8> m_visibleBG;

    uint32 m_cachedWidth;
    uint32 m_cachedHeight;
    Renderer2D m_renderer;
};