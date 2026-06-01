#pragma once

#include "../Precompiled.h"
#include <cmath>
#include "Vector2.h"

#undef M_PI
static const float M_PI = 3.14159265358979323846264338327950288f;

template<typename T, typename U>
inline T Min(T lhs, U rhs) { return lhs < rhs ? lhs : rhs; }

template <typename T, typename U>
inline T Max(T lhs, U rhs) { return lhs > rhs ? lhs : rhs; }

template<typename T>
inline T Round(T x) { return floor(x + T(0.5)); }

template<typename T>
inline T Clamp(T value, T min, T max)
{
    if(value < min) {
        return min;
    }
    else if(value > max) {
        return max;
    }

    return value;
}

template <typename T> 
inline T Sqrt(T x) { return sqrt(x); }

template<typename T>
inline T Abs(T value) { return value >= 0.0 ? value : -value; }

template <class T> 
inline T Atan2(T y, T x) { return atan2(y, x) * 180.0f / M_PI; }

template <class T> 
inline T Sin(T angle) { return sin(angle * M_PI / 180.0f); }

template <class T> 
inline T Cos(T angle) { return cos(angle * M_PI / 180.0f); }

template<typename T>
inline float DistanceBetweenPoints(const Vector2<T>& p1, const Vector2<T>& p2)
{
    T dx = p1.x - p2.x;
    T dy = p1.y - p2.y;

    return sqrt((dx * dx) + (dy * dy));
}

template<typename T>
inline bool IsPointInPolygon(const std::vector<Vector2<T>>& polygon, const Vector2<T> point)
{
    bool inside = false;

    int32 n = (int32)polygon.size();
    if(n < 3) 
        return false;

    for(int32 i = 0, j = n - 1; i < n; j = i++)
    {
        const Vector2<T>& p0 = polygon[i];
        const Vector2<T>& p1 = polygon[j];

        if(((p0.y <= point.y) && (p1.y > point.y)) || ((p1.y <= point.y) && (p0.y > point.y)))
        {
            T cross = (p1.x - p0.x) * (point.y - p0.y) / (p1.y - p0.y) + p0.x;

            if (cross < point.x)
            {
                inside = !inside;
            }
        }
    }

    return inside;
}

inline bool IsNan(float value) { return std::isnan(value); }