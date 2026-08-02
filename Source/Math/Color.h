#pragma once

#include "../Precompiled.h"

#define TO_COLOR_RED(x) ((x >> 8) & 0xFF)
#define TO_COLOR_GREEN(x) ((x >> 16) & 0xFF)
#define TO_COLOR_BLUE(x) ((x >> 24) & 0xFF)
#define TO_COLOR_RGB(r, g, b) (((uint32)(r) << 8) | ((uint32)(g) << 16) | ((uint32)(b) << 24))
#define TO_COLOR_RGBA(r, g, b, a) (((uint32)(r) << 8) | ((uint32)(g) << 16) | ((uint32)(b) << 24) | (uint32)(a))

class Color
{
public:
    Color() : r(255), g(255), b(255), a(255) {}
    Color(uint8 r_, uint8 g_, uint8 b_) : r(r_), g(g_), b(b_), a(255) {}
    Color(uint8 r_, uint8 g_, uint8 b_, uint8 a_) : r(r_), g(g_), b(b_), a(a_) {}
    Color(uint32 color) { SetUINT(color); }

    uint32 GetAsUINTSwap() { return ((uint32)b << 24) | ((uint32)g << 16) | ((uint32)r << 8) | ((uint32)a); }

    uint32 GetAsUINT() { return ((uint32)r << 24) | ((uint32)g << 16) | ((uint32)b << 8) | ((uint32)a); }

    void SetUINT(uint32 color)
    {
        r = (color >> 24) & 0xFF;
        g = (color >> 16) & 0xFF;
        b = (color >> 8) & 0xFF;
        a = (color) & 0xFF;
    }

    void SetUINTSwap(uint32 color)
    {
        b = (color >> 24) & 0xFF;
        g = (color >> 16) & 0xFF;
        r = (color >> 8) & 0xFF;
        a = (color) & 0xFF;
    }

public:
    uint8 r, g, b, a;

public:
    static const Color New;
};