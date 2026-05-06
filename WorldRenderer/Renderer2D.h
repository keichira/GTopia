#pragma once

#include "Precompiled.h"
#include <blend2d/blend2d.h>

class Renderer2D {
public:
    Renderer2D();
    ~Renderer2D();

public:
    void Init(int32 width, int32 height);
    void SetThreadCount(uint8 threadCount) { m_threadCount = threadCount; }

    void DrawRect(const BLRect& rect, const BLRgba32& color = BLRgba32(255, 255, 255, 255));
    void DrawImage(BLImage* pImage, const  BLRect& drawRect, const BLRgba32& color = BLRgba32(255, 255, 255, 255), float rotateAngle = 0.0f);
    void DrawSprite(BLImage* pImage, const BLRect& drawRect, const BLRectI& spriteRect, const BLRgba32& color = BLRgba32(255, 255, 255, 255), float rotateAngle = 0.0f);
    void DrawText(BLFont* pFont, const BLPoint& origin, const string& text, const BLRgba32& color = BLRgba32(255, 255, 255, 255), float size = 16.0f, float rotateAngle = 0.0f);

    void DrawGTText(BLFont* pFont, const BLPoint& origin, const string& text, float size = 16.0f);

    void End() { m_context.end(); }
    bool WriteToFile(const string& path);

    float GetTextWidth(BLFont* pFont, const string& text, float size);
    float GetTextHeight(BLFont* pFont, float size);

    int32 GetSurfaceWitdh() const { return m_surface.width(); }
    int32 GetSurfaceHeight() const { return m_surface.height(); }

private:
    BLImage TintSprite(BLImage* src, const BLRectI& rect, const BLRgba32& color);

private:
    BLImage m_surface;
    BLContext m_context;
    uint8 m_threadCount;
};