#pragma once

#include <math.h>
#include "Vector2.h"

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
inline float DistanceBetweenPoints(Vector2<T> p1, Vector2<T> p2)
{
    T dx = p1.x - p2.x;
    T dy = p1.y - p2.y;

    return sqrt((dx * dx) + (dy * dy));
}

inline bool IsNan(float value) { return std::isnan(value); }