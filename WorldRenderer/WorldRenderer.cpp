#include "WorldRenderer.h"
#include "IO/File.h"
#include "Context.h"
#include "Utils/StringUtils.h"
#include "Utils/ResourceManager.h"
#include "IO/Log.h"
#include "WorldTileLut.h"
#include "Math/Random.h"
#include "WeatherRenderer.h"

WorldRenderer::WorldRenderer()
{
}

WorldRenderer::~WorldRenderer()
{
}

bool WorldRenderer::LoadWorld(uint32 worldID)
{
    string path = GetContext()->GetGameConfig()->worldSavePath + "/world_" + ToString(worldID) + ".bin";

    File file;
    if(!file.Open(path)) {
        return false;
    }

    uint32 fileSize = file.GetSize();
    uint8* pData = new uint8[fileSize];

    if(file.Read(pData, fileSize) != fileSize) {
        SAFE_DELETE_ARRAY(pData);
        file.Close();
        return false;
    }

    MemoryBuffer memBuffer(pData, fileSize);
    m_world.Serialize(memBuffer, false, true);
    SAFE_DELETE_ARRAY(pData);
    file.Close();

    return true;
}

void WorldRenderer::Draw()
{
    m_renderer.SetThreadCount(2);

    WorldTileManager* pTileMgr = m_world.GetTileManager();

    uint32 worldWidth = pTileMgr->GetSize().x;
    uint32 worldHeight = pTileMgr->GetSize().y;

    m_renderer.Init(worldWidth * 16, worldHeight * 16);

    DrawWeather();
    ComputeVisibleBG();

    for(int32 y = 0; y < worldHeight; ++y) {
        for(int32 x = 0; x < worldWidth; ++x) {
    
            TileInfo* pTile = pTileMgr->GetTile(x, y);
            if(!pTile)
                continue;
    
            int32 idx = y * worldWidth + x;
    
            if(pTile->GetBG() != ITEM_ID_BLANK && m_visibleBG[idx]) {
                DrawTile(pTile, pTile->GetBG());
            }
    
            if(pTile->GetFG() != ITEM_ID_BLANK) {
                DrawTile(pTile, pTile->GetFG());
            }
        }
    }

    BLRect bedrockRect(0, (pTileMgr->GetSize().y - 6) * 16, pTileMgr->GetSize().x * 16, 6 * 16);
    m_renderer.DrawRect(bedrockRect, BLRgba32(0, 0, 0, 150));

    BLFont* pFont = GetResourceManager()->GetFont(FONT_TYPE_CENTURY_GOTHIC_BOLD);
    if(pFont) {
        string worldName = "`#Visit `0\"" + m_world.GetWorlName() + "\" `#in";

        float textWidth = m_renderer.GetTextWidth(pFont, worldName, 32);
        float textHeight = m_renderer.GetTextHeight(pFont, 32);


        m_renderer.DrawGTText(pFont, BLPoint{m_renderer.GetSurfaceWitdh() - textWidth * 0.9f, m_renderer.GetSurfaceHeight() - textHeight - 16*3}, worldName, 32);
    }

    m_renderer.WriteToFile(GetContext()->GetGameConfig()->rendererSavePath + "/" + m_world.GetWorlName() + ".png");
}

void WorldRenderer::ComputeVisibleBG()
{
    WorldTileManager* pTileMgr = m_world.GetTileManager();
    uint32 worldWidth = pTileMgr->GetSize().x;
    uint32 worldHeight = pTileMgr->GetSize().y;

    m_visibleBG.clear();
    m_visibleBG.resize(worldWidth * worldHeight, 0);

    int32 neighbors[5][2] = 
    {
        {0, 0}, {0, -1}, {-1, 0}, {1, 0}, {0, 1}
    };

    for(int32 y = 0; y < worldHeight; ++y) {
        for(int32 x = 0; x < worldWidth; ++x) {
            TileInfo* pTile = pTileMgr->GetTile(x, y);
            if(!pTile) {
                continue;
            }

            if(
                IsFGTransparent(pTile->GetFG()) ||
                ((worldHeight - y) >= 6 && pTile->GetFG() == ITEM_ID_BEDROCK)
            ) {
                for(auto& n : neighbors) {
                    int32 tileX = x + n[0];
                    int32 tileY = y + n[1];

                    if(tileX >= 0 && tileY >= 0 && tileX < worldWidth && tileY < worldHeight) {
                        int32 idx = tileY * worldWidth + tileX;
                        m_visibleBG[idx] = 1;
                    }
                }
            }
        }
    }
}

void WorldRenderer::DrawWeather()
{
    WeatherRenderer weatherRenderer(m_renderer);

    switch(m_world.GetCurrentWeather()) {
        case WEATHER_TYPE_SUNSET: {
            weatherRenderer.DrawSunset();
            break;
        }

        case WEATHER_TYPE_HARVEST: {
            weatherRenderer.DrawHarvest();
            break;
        }

        case WEATHER_TYPE_SUNNY:
        case WEATHER_TYPE_DEFAULT:
        default: {
            weatherRenderer.DrawDefault();
        }
    }
}

void WorldRenderer::DrawTile(TileInfo* pTile, uint16 itemID)
{
    if(!pTile) {
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if(!pItem) {
        return;
    }

    BLImage* pImg = GetResourceManager()->GetItemTileSheet(itemID);
    if(!pImg) {
        return;
    }

    Vector2Int tilePos = pTile->GetPos();
    Vector2Int spriteCoords = GetSpriteCoords(pTile, pItem);

    BLRect drawRect(tilePos.x * 16, tilePos.y * 16, 16, 16);
    BLRectI spriteRect(spriteCoords.x * 32, spriteCoords.y * 32, 32, 32);

    m_renderer.DrawSprite(pImg, drawRect, spriteRect);
}

Vector2Int WorldRenderer::GetSpriteCoords(TileInfo* pTile, ItemInfo* pItem)
{
    if(!pItem) {
        return { 0, 0 }; 
    }

    uint8 coordX = pItem->textureX;
    uint8 coordY = pItem->textureY;

    Vector2Int tilePos = pTile->GetPos();
    WorldTileManager* pTileMgr = m_world.GetTileManager();

    Vector2Int worldSize = pTileMgr->GetSize();

    bool isBackground = pItem->IsBackground();

    switch(pItem->storage) {
        case STORAGE_SMART_EDGE: {
            bool topLeft = 
                (tilePos.x > 0 && tilePos.y > 0) ? pTileMgr->IsSameTile(pTile, tilePos.x - 1, tilePos.y - 1 , isBackground) : false;

            bool midLeft = 
                (tilePos.x > 0) ? pTileMgr->IsSameTile(pTile, tilePos.x - 1, tilePos.y, isBackground) : false;
            
            bool botLeft = 
                (tilePos.x > 0 && tilePos.y < worldSize.y - 1) ? pTileMgr->IsSameTile(pTile, tilePos.x - 1, tilePos.y + 1, isBackground) : false;

            bool bottom = 
                (tilePos.y < worldSize.y - 1) ?  pTileMgr->IsSameTile(pTile, tilePos.x, tilePos.y + 1, isBackground) : false;

            bool bottomRight = 
                (tilePos.x < worldSize.x - 1 && tilePos.y < worldSize.y - 1) ? pTileMgr->IsSameTile(pTile, tilePos.x + 1, tilePos.y + 1, isBackground) : false; 

            bool midRight = 
                (tilePos.x < worldSize.x - 1) ? pTileMgr->IsSameTile(pTile, tilePos.x + 1, tilePos.y, isBackground) : false; 

            bool topRight = 
                (tilePos.x < worldSize.x - 1 && tilePos.y > 0) ? pTileMgr->IsSameTile(pTile, tilePos.x + 1, tilePos.y - 1, isBackground) : false; 

            bool top = 
                (tilePos.y > 0) ? pTileMgr->IsSameTile(pTile, tilePos.x, tilePos.y - 1, isBackground) : false; 

            uint8 mask = (topLeft << 0) | (top << 1) | (topRight << 2) |
                        (midRight << 3) | (bottomRight << 4) | (bottom << 5) |
                        (botLeft << 6) | (midLeft << 7);

            int32 lutVisual = sWorldTileSmartEdgeLut[mask];

            coordX += lutVisual % 8;
            coordY += lutVisual / 8;
            break;
        }

        case STORAGE_SMART_EDGE_HORIZ: {
            bool right = pTileMgr->IsSameTile(pTile, tilePos.x + 1, tilePos.y, isBackground);
            bool left = pTileMgr->IsSameTile(pTile, tilePos.x - 1, tilePos.y, isBackground);

            if(right) {
                coordX += left ? 1 : 0;
            }
            else {
                coordX += left ? 0 : 3;
            }
            break;
        }

        case STORAGE_SMART_CLING: {
            TileInfo* pTop = pTileMgr->GetTile(tilePos.x, tilePos.y + 1);
            TileInfo* pLeft = pTileMgr->GetTile(tilePos.x - 1, tilePos.y);
            TileInfo* pRight = pTileMgr->GetTile(tilePos.x + 1, tilePos.y);
            TileInfo* pBottom = pTileMgr->GetTile(tilePos.x + 1, tilePos.y + 1); 
            
            break;
        }

        case STORAGE_SMART_CLING2: {
            bool top = 
                (tilePos.y > 0) ? pTileMgr->IsSameTile(pTile, tilePos.x, tilePos.y - 1, isBackground) : false;

            bool left = 
                (tilePos.x > 0) ? pTileMgr->IsSameTile(pTile, tilePos.x - 1, tilePos.y, isBackground) : false;

            bool right = 
                (tilePos.x < worldSize.x - 1) ? pTileMgr->IsSameTile(pTile, tilePos.x + 1, tilePos.y, isBackground) : false;

            bool bottom = 
                (tilePos.y < worldSize.y - 1) ? pTileMgr->IsSameTile(pTile, tilePos.x, tilePos.y + 1, isBackground) : false; 

            uint8 mask = (top << 0) | (left << 1) | (right << 2) | (bottom << 3);
            int32 lutVisual = sWorldTileSmartEdgeOuterLut[mask];

            coordX += lutVisual % 8;
            coordY += lutVisual / 8;
            break;
        }

        case STORAGE_SMART_EDGE_VERT: {
            bool top = pTileMgr->IsSameTile(pTile, tilePos.x, tilePos.y + 1, isBackground);
            bool bottom = pTileMgr->IsSameTile(pTile, tilePos.x, tilePos.y + 1, isBackground);

            if(top) {
                coordX += bottom ? 1 : 0;
            }
            else {
                coordX += bottom ? 0 : 3;
            }
            break;
        }

        case STORAGE_RANDOM: {
            coordX += (RandomRangeInt(0, 3));
            break;
        }
    }

    if(pTile->HasFlag(TILE_FLAG_IS_ON)) {
        coordX += 1;
    }

    return { coordX, coordY };
}

bool WorldRenderer::IsFGTransparent(uint16 itemID)
{
    switch(itemID) {
        case ITEM_ID_BLANK:
        case ITEM_ID_LAVA:
        //case ITEM_ID_BEDROCK: // confuse
        case ITEM_ID_ROCK: {
            return true;
        }

        default: {
            return false;
        }
    }
}
