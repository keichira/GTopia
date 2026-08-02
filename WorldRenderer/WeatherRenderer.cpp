#include "WeatherRenderer.h"
#include "Math/Math.h"
#include "Math/Random.h"
#include "Utils/ResourceManager.h"

WeatherRenderer::WeatherRenderer(const Renderer2D& renderer) : m_renderer(const_cast<Renderer2D&>(renderer)) {}

WeatherRenderer::~WeatherRenderer() {}

void WeatherRenderer::DrawDefault()
{
    ResourceManager* pResMgr = GetResourceManager();
    int32 surfaceWidth = m_renderer.GetSurfaceWitdh();
    int32 surfaceHeight = m_renderer.GetSurfaceHeight();

    BLRect backgroundRect(0, 0, surfaceWidth, surfaceHeight);
    m_renderer.DrawRect(backgroundRect, BLRgba32(96, 215, 242, 255));

    BLImage* pSunImg = pResMgr->GetTileSheet("game/sun.rttex");
    if (pSunImg)
    {
        float sunScale = (surfaceWidth / 161.0f) / 6.0f;
        BLRect sunRect(surfaceWidth * 0.8, surfaceHeight * 0.1, pSunImg->width() * sunScale,
                       pSunImg->height() * sunScale);
        m_renderer.DrawImage(pSunImg, sunRect);
    }

    float hillScale = surfaceWidth * (1.0f / 512.0f);

    BLImage* pHills3 = pResMgr->GetTileSheet("game/hills3.rttex");
    if (pHills3)
    {
        int32 imgW = pHills3->width() * hillScale;
        int32 imgH = pHills3->height() * hillScale;

        BLRect hills1Rect(0, surfaceHeight - imgH - 60 * hillScale, imgW, imgH);
        m_renderer.DrawImage(pHills3, hills1Rect);
    }

    BLImage* pHills2 = pResMgr->GetTileSheet("game/hills2.rttex");
    if (pHills2)
    {
        int32 imgW = pHills2->width() * hillScale;
        int32 imgH = pHills2->height() * hillScale;

        BLRect hills1Rect(0, surfaceHeight - imgH + 46 * hillScale, imgW, imgH);
        m_renderer.DrawImage(pHills2, hills1Rect);
    }

    BLImage* pHills1 = pResMgr->GetTileSheet("game/hills1.rttex");
    if (pHills1)
    {
        int32 imgW = pHills1->width() * hillScale;
        int32 imgH = pHills1->height() * hillScale;

        BLRect hills1Rect(0, surfaceHeight - imgH, imgW, imgH);
        m_renderer.DrawImage(pHills1, hills1Rect);
    }

    // DrawClouds();
}

void WeatherRenderer::DrawClouds()
{
    ResourceManager* pResMgr = GetResourceManager();
    BLImage* pCloud = pResMgr->GetTileSheet("game/cloud.rttex");
    if (!pCloud)
    {
        return;
    }

    int32 cloudCount = RandomRangeInt(4, 8);
    int32 surfaceWidth = m_renderer.GetSurfaceWitdh();
    int32 surfaceHeight = m_renderer.GetSurfaceHeight();

    int32 sectionWidth = surfaceWidth / cloudCount;

    for (int32 i = 0; i < cloudCount; ++i)
    {
        float scale = RandomRangeFloat(0.4f, 2.0f);
        float randX = RandomRangeFloat(i * sectionWidth, (i + 1) * sectionWidth);
        float randY = RandomRangeFloat(surfaceHeight * 0.05f, surfaceHeight * 0.17f);

        BLRect drawRect(randX, randY, pCloud->width() * scale, pCloud->height() * scale);

        float norm = (scale - 0.4f) / (2.0f - 0.4f);
        BLRgba32 cloudColor(Clamp(int32(200 + norm * 55), 0, 255), Clamp(int32(200 + norm * 55), 0, 255),
                            Clamp(int32(220 + norm * 55), 0, 255), 255);

        m_renderer.DrawImage(pCloud, drawRect, cloudColor);
    }
}

void WeatherRenderer::DrawHarvest()
{
    ResourceManager* pResMgr = GetResourceManager();
    int32 surfaceWidth = m_renderer.GetSurfaceWitdh();
    int32 surfaceHeight = m_renderer.GetSurfaceHeight();

    BLImage* pBackground = pResMgr->GetTileSheet("game/sunset.rttex");
    if (pBackground)
    {
        BLRect bgRect(0, -10.f, pBackground->width() * surfaceWidth * 0.3f,
                      pBackground->height() * (surfaceHeight / 28.0f));
        m_renderer.DrawImage(pBackground, bgRect);
    }

    BLImage* pSunImg = pResMgr->GetTileSheet("game/moon2.rttex");
    if (pSunImg)
    {
        float sunScale = (surfaceWidth * 0.003f) / 2.5f;
        BLRect sunRect(surfaceWidth * 0.7, surfaceHeight * 0.15, pSunImg->width() * sunScale,
                       pSunImg->height() * sunScale);
        m_renderer.DrawImage(pSunImg, sunRect);
    }

    /**
     * ehh lol add fences
     */
}

void WeatherRenderer::DrawSunset()
{
    ResourceManager* pResMgr = GetResourceManager();
    int32 surfaceWidth = m_renderer.GetSurfaceWitdh();
    int32 surfaceHeight = m_renderer.GetSurfaceHeight();

    BLImage* pBackground = pResMgr->GetTileSheet("game/sunset.rttex");
    if (pBackground)
    {
        BLRect bgRect(0, 0, pBackground->width() * surfaceWidth * 0.3f,
                      pBackground->height() * (surfaceHeight * 0.15f));
        m_renderer.DrawImage(pBackground, bgRect);
    }
}
