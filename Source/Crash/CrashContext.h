#pragma once

#include "../OS/OSPrecompiled.h"

class CrashContext
{
public:
    template <typename T, typename std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
    static void Set(const char* key, T value)
    {
        SetInt64(key, (int64)(value));
    }

    template <typename T, typename std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
    static void Set(const char* key, T value)
    {
        SetDouble(key, (double)(value));
    }

    static void Set(const char* key, bool value);
    static void Set(const char* key, const char* value);

    static usize Dump(char* outBuffer, usize bufferSize);

private:
    static void SetInt64(const char* key, int64 value);
    static void SetDouble(const char* key, double value);
};

#define CRASH_SET(key, val) CrashContext::Set(key, val)