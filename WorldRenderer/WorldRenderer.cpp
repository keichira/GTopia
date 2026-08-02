#include "WorldRenderer.h"
#include "Context.h"
#include "IO/File.h"
#include "Utils/ResourceManager.h"
#include "Utils/StringUtils.h"
#include "WeatherRenderer.h"
#include "WorldTileLut.h"

WorldRenderer::WorldRenderer() : m_cachedWidth(0), m_cachedHeight(0) {}
WorldRenderer::~WorldRenderer() {}

bool WorldRenderer::LoadWorld(uint32 worldID, WorldInfo* pOutWorld)
{
    if (!pOutWorld)
        return false;

    string path = GetContext()->GetGameConfig()->worldSavePath + "/world_" + ToString(worldID) + ".bin";
    File file;
    if (!file.Open(path))
        return false;

    uint32 fileSize = file.GetSize();
    uint8* pData = new uint8[fileSize];

    if (file.Read(pData, fileSize) != fileSize)
    {
        SAFE_DELETE_ARRAY(pData);
        file.Close();
        return false;
    }

    MemoryBuffer memBuffer(pData, fileSize);
    pOutWorld->Serialize(memBuffer, false, true);

    SAFE_DELETE_ARRAY(pData);
    file.Close();
    return true;
}

void WorldRenderer::Draw(WorldInfo* pWorld)
{
    if (!pWorld)
        return;

    m_renderer.SetThreadCount(2);

    WorldTileManager* pTileMgr = pWorld->GetTileManager();
    uint32 worldWidth = pTileMgr->GetSize().x;
    uint32 worldHeight = pTileMgr->GetSize().y;

    uint32 targetWidth = worldWidth * 16;
    uint32 targetHeight = worldHeight * 16;

    if (m_cachedWidth != targetWidth || m_cachedHeight != targetHeight)
    {
        m_renderer.Init(targetWidth, targetHeight);
        m_cachedWidth = targetWidth;
        m_cachedHeight = targetHeight;
    }
    else
    {
        m_renderer.Clear();
    }

    m_renderer.Begin();

    DrawWeather(pWorld);
    ComputeVisibleBG(pWorld);

    for (int32 y = 0; y < (int32)worldHeight; ++y)
    {
        for (int32 x = 0; x < (int32)worldWidth; ++x)
        {
            TileInfo* pTile = pTileMgr->GetTile(x, y);
            if (!pTile)
                continue;

            int32 idx = y * worldWidth + x;

            if (pTile->GetBG() != ITEM_ID_BLANK && m_visibleBG[idx])
            {
                DrawTile(pWorld, pTile, pTile->GetBG());
            }

            if (pTile->GetFG() != ITEM_ID_BLANK)
            {
                DrawTile(pWorld, pTile, pTile->GetFG());
            }
        }
    }

    BLRect bedrockRect(0, (pTileMgr->GetSize().y - 6) * 16, pTileMgr->GetSize().x * 16, 6 * 16);
    m_renderer.DrawRect(bedrockRect, BLRgba32(0, 0, 0, 150));

    BLFont* pFont = GetResourceManager()->GetFont(FONT_TYPE_CENTURY_GOTHIC_BOLD);
    if (pFont)
    {
        string worldName = "`#Visit `0\"" + pWorld->GetWorlName() + "\" `#in";
        float textWidth = m_renderer.GetTextWidth(pFont, worldName, 32);
        float textHeight = m_renderer.GetTextHeight(pFont, 32);

        m_renderer.DrawGTText(pFont,
                              BLPoint{m_renderer.GetSurfaceWitdh() - textWidth * 0.9f,
                                      m_renderer.GetSurfaceHeight() - textHeight - 16 * 3},
                              worldName, 32);
    }

    m_renderer.WriteToFile(GetContext()->GetGameConfig()->rendererSavePath + "/" + pWorld->GetWorlName() + ".png");
    m_renderer.End();
}

void WorldRenderer::ComputeVisibleBG(WorldInfo* pWorld)
{
    WorldTileManager* pTileMgr = pWorld->GetTileManager();
    uint32 worldWidth = pTileMgr->GetSize().x;
    uint32 worldHeight = pTileMgr->GetSize().y;

    m_visibleBG.clear();
    m_visibleBG.resize(worldWidth * worldHeight, 0);

    int32 neighbors[5][2] = {{0, 0}, {0, -1}, {-1, 0}, {1, 0}, {0, 1}};

    for (int32 y = 0; y < (int32)worldHeight; ++y)
    {
        for (int32 x = 0; x < (int32)worldWidth; ++x)
        {
            TileInfo* pTile = pTileMgr->GetTile(x, y);
            if (!pTile)
                continue;

            if (IsFGTransparent(pTile->GetFG()) || ((worldHeight - y) >= 6 && pTile->GetFG() == ITEM_ID_BEDROCK))
            {
                for (auto& n : neighbors)
                {
                    int32 tileX = x + n[0];
                    int32 tileY = y + n[1];

                    if (tileX >= 0 && tileY >= 0 && tileX < (int32)worldWidth && tileY < (int32)worldHeight)
                    {
                        int32 idx = tileY * worldWidth + tileX;
                        m_visibleBG[idx] = 1;
                    }
                }
            }
        }
    }
}

void WorldRenderer::DrawWeather(WorldInfo* pWorld)
{
    WeatherRenderer weatherRenderer(m_renderer);
    switch (pWorld->GetCurrentWeather())
    {
        case WEATHER_TYPE_SUNSET:
            weatherRenderer.DrawSunset();
            break;

        case WEATHER_TYPE_HARVEST:
            weatherRenderer.DrawHarvest();
            break;

        default:
            weatherRenderer.DrawDefault();
            break;
    }
}

void WorldRenderer::DrawTile(WorldInfo* pWorld, TileInfo* pTile, int16 itemID)
{
    if (!pTile)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
        return;

    BLImage* pImg = GetResourceManager()->GetItemTileSheet(itemID);
    if (!pImg)
        return;

    Vector2Int tilePos = pTile->GetPos();
    Vector2Int spriteCoords = GetSpriteCoords(pWorld, pTile, pItem);

    BLRect drawRect(tilePos.x * 16, tilePos.y * 16, 16, 16);
    BLRectI spriteRect(spriteCoords.x * 32, spriteCoords.y * 32, 32, 32);

    m_renderer.DrawSprite(pImg, drawRect, spriteRect);
}

void WorldRenderer::DrawTileShadows(WorldInfo* pWorld)
{
    if (!pWorld)
        return;

    WorldTileManager* pTileMgr = pWorld->GetTileManager();

    Vector2Int& vWorldSize = pTileMgr->GetSize();

    for (int32 i = 0; i < (vWorldSize.x * vWorldSize.y); ++i)
    {
        TileInfo* pTile = pTileMgr->GetTile(i);
        if (!pTile)
            continue;
    }
}

Vector2Int WorldRenderer::GetSpriteCoords(WorldInfo* pWorld, TileInfo* pTile, ItemInfo* pItem)
{
    if (!pItem)
        return {0, 0};

    uint8 coordX = pItem->textureX;
    uint8 coordY = pItem->textureY;

    Vector2Int tilePos = pTile->GetPos();
    WorldTileManager* pTileMgr = pWorld->GetTileManager();

    Vector2Int worldSize = pTileMgr->GetSize();

    bool isBackground = pItem->IsBackground();

    switch (pItem->storage)
    {
        case STORAGE_SMART_EDGE:
        {
            bool topLeft = (tilePos.x > 0 && tilePos.y > 0)
                               ? pTileMgr->IsSameTile(pTile, tilePos.x - 1, tilePos.y - 1, isBackground)
                               : false;

            bool midLeft =
                (tilePos.x > 0) ? pTileMgr->IsSameTile(pTile, tilePos.x - 1, tilePos.y, isBackground) : false;

            bool botLeft = (tilePos.x > 0 && tilePos.y < worldSize.y - 1)
                               ? pTileMgr->IsSameTile(pTile, tilePos.x - 1, tilePos.y + 1, isBackground)
                               : false;

            bool bottom = (tilePos.y < worldSize.y - 1)
                              ? pTileMgr->IsSameTile(pTile, tilePos.x, tilePos.y + 1, isBackground)
                              : false;

            bool bottomRight = (tilePos.x < worldSize.x - 1 && tilePos.y < worldSize.y - 1)
                                   ? pTileMgr->IsSameTile(pTile, tilePos.x + 1, tilePos.y + 1, isBackground)
                                   : false;

            bool midRight = (tilePos.x < worldSize.x - 1)
                                ? pTileMgr->IsSameTile(pTile, tilePos.x + 1, tilePos.y, isBackground)
                                : false;

            bool topRight = (tilePos.x < worldSize.x - 1 && tilePos.y > 0)
                                ? pTileMgr->IsSameTile(pTile, tilePos.x + 1, tilePos.y - 1, isBackground)
                                : false;

            bool top = (tilePos.y > 0) ? pTileMgr->IsSameTile(pTile, tilePos.x, tilePos.y - 1, isBackground) : false;

            uint8 mask = (topLeft << 0) | (top << 1) | (topRight << 2) | (midRight << 3) | (bottomRight << 4) |
                         (bottom << 5) | (botLeft << 6) | (midLeft << 7);

            int32 lutVisual = sWorldTileSmartEdgeLut[mask];

            coordX += lutVisual % 8;
            coordY += lutVisual / 8;
            break;
        }

        case STORAGE_SMART_EDGE_HORIZ:
        {
            bool right = pTileMgr->IsSameTile(pTile, tilePos.x + 1, tilePos.y, isBackground);
            bool left = pTileMgr->IsSameTile(pTile, tilePos.x - 1, tilePos.y, isBackground);

            if (right)
            {
                coordX += left ? 1 : 0;
            }
            else
            {
                coordX += left ? 0 : 3;
            }
            break;
        }

        case STORAGE_SMART_EDGE_DIAGON:
        {
            bool isFlipped = (pTile->HasFlag(TILE_FLAG_FLIPPED_X));

            int32 diag1X = 0, diag1Y = 0;
            int32 diag2X = 0, diag2Y = 0;

            if (!isFlipped)
            {
                diag1X = tilePos.x - 1;
                diag1Y = tilePos.y + 1;
                diag2X = tilePos.x + 1;
                diag2Y = tilePos.y - 1;
            }
            else
            {
                diag1X = tilePos.x + 1;
                diag1Y = tilePos.y + 1;
                diag2X = tilePos.x - 1;
                diag2Y = tilePos.y - 1;
            }

            auto checkDiagonal = [&](int32 tx, int32 ty) -> bool
            {
                if (tx < 0 || tx >= worldSize.x || ty < 0 || ty >= worldSize.y)
                    return true;

                TileInfo* pTarget = pTileMgr->GetTile(tx, ty);
                if (!pTarget)
                    return true;

                uint16 currentID = isBackground ? pTile->GetBG() : pTile->GetFG();
                uint16 targetID = isBackground ? pTarget->GetBG() : pTarget->GetFG();

                if (targetID != ITEM_ID_BLANK && pTarget->HasFlag(TILE_FLAG_GLUED))
                {
                    if (!isBackground && (targetID & 1) == 0)
                    {
                        return true;
                    }
                    if (isBackground)
                    {
                        return true;
                    }
                }

                if (targetID == currentID)
                {
                    return pTarget->HasFlag(TILE_FLAG_FLIPPED_X) == isFlipped;
                }

                return false;
            };

            bool bVar2 = checkDiagonal(diag1X, diag1Y);
            bool bVar3 = checkDiagonal(diag2X, diag2Y);

            static const uint8 diagonalLut[2][2] = {{3, 0}, {2, 1}};

            int32 lutVisual = diagonalLut[bVar2][bVar3];

            coordX += lutVisual % 8;
            coordY += lutVisual / 8;
            break;
        }

        case STORAGE_SMART_EDGE_OUTER:
        {
            uint16 currentID = isBackground ? pTile->GetBG() : pTile->GetFG();

            auto checkOuterConnection = [&](int32 tx, int32 ty, int32 direction) -> bool
            {
                if (tx < 0 || tx >= worldSize.x || ty < 0 || ty >= worldSize.y)
                    return true;

                TileInfo* pTarget = pTileMgr->GetTile(tx, ty);
                if (!pTarget)
                    return true;

                if (isBackground)
                {
                    uint16 targetBG = pTarget->GetFG();

                    if (targetBG == ITEM_ID_BLANK || !pTarget->HasFlag(TILE_FLAG_GLUED))
                        return false;

                    if (currentID == ITEM_ID_WEEPING_WILLOW_BRANCH)
                        return (targetBG == ITEM_ID_WEEPING_WILLOW);

                    return (targetBG == currentID);
                }
                else
                {
                    int32 checkType = (direction == 0 || direction == 2) ? 0 : ((direction == 1) ? 1 : 2);
                    return (pTarget->GetFG() == currentID);
                }
            };

            bool right = checkOuterConnection(tilePos.x + 1, tilePos.y, 0);
            bool bottom = checkOuterConnection(tilePos.x, tilePos.y + 1, 1);
            bool left = checkOuterConnection(tilePos.x - 1, tilePos.y, 0);
            bool top = checkOuterConnection(tilePos.x, tilePos.y - 1, 2);

            uint8 mask = (right << 0) | (bottom << 1) | (left << 2) | (top << 3);
            int32 lutVisual = sWorldTileSmartEdgeOuterLut[mask];

            coordX += lutVisual % 8;
            coordY += lutVisual / 8;
            break;
        }

        case STORAGE_SMART_EDGE_VERT:
        {
            bool top = pTileMgr->IsSameTile(pTile, tilePos.x, tilePos.y - 1, isBackground);
            bool bottom = pTileMgr->IsSameTile(pTile, tilePos.x, tilePos.y + 1, isBackground);

            if (top)
            {
                coordX += bottom ? 1 : 0;
            }
            else
            {
                coordX += bottom ? 0 : 3;
            }
            break;
        }
    }

    if (pTile->HasFlag(TILE_FLAG_IS_ON))
    {
        coordX += 1;
    }

    return {coordX, coordY};
}

bool WorldRenderer::IsFGTransparent(int16 itemID)
{
    switch (itemID)
    {
        case ITEM_ID_BLANK:
        case ITEM_ID_LAVA:
        // case ITEM_ID_BEDROCK: // confuse
        case ITEM_ID_ROCK:
        {
            return true;
        }

        default:
        {
            return false;
        }
    }
}
