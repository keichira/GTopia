#pragma once

#include <math.h>

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

inline bool IsNan(float value) { return std::isnan(value); }