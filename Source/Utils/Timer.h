#pragma once

#include "../Precompiled.h"

class Timer {
public:
    Timer();

public:
    void Reset();
    void Reset(uint64 newTime);
    void Set(uint64 ms);
    bool IsPassed();
    uint64 GetElapsedTime(bool reset = false);
    uint64 GetStartTime() const;

private:
    uint64 m_startTime;
};

class Time {
public:
    Time();

public:
    static uint64 GetSystemTime();
    static uint64 GetTimeSinceEpoch();
    static uint32 GetSecondsFromMidnight();
    static string GetDateTimeStr();
    static string ConvertTimeToStr(uint64 ms);

private:
    Timer m_timer;
};