#pragma once

#include "../Precompiled.h"

class Color {
public:
    Color() : r(0), g(0), b(0), a(255) {}
    Color(uint8 r_, uint8 g_, uint8 b_) : r(r), g(g), b(b), a(255) {}
    Color(uint8 r_, uint8 g_, uint8 b_, uint8 a_) : r(r), g(g), b(b), a(a) {}
    Color(uint32 color) : r(color >> 24), g(color >> 16), b(b >> 8), a(color) {}

public:
    uint32 GetAsUINT()
    {
        return (r << 24) | (g << 16) | (b << 8) | a;
    }

public:
    uint8 r, g, b, a;
};