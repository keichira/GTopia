#pragma once

#include "../Precompiled.h"

class Timer
{
public:
    Timer();

public:
    void Reset();
    void Reset(uint64 newTime);
    void Set(uint64 ms);
    void UnSet();
    bool IsPassed();
    uint64 GetRemainingTime();
    uint64 GetElapsedTime(bool reset = false);
    uint64 GetStartTime() const;

private:
    uint64 m_startTime;
    bool m_isSet;
};

class Time
{
public:
    Time();

public:
    static uint64 GetSystemTime();
    static uint64 GetTimeSinceEpoch();
    static uint32 GetSecondsFromMidnight();
    static string GetDateTimeStr();
    static string ConvertTimeToStr(uint64 ms);
    static int32 GetDaysInMonth(int32 year, int32 month);
    static string FormatTimeToStr(time_t targetTime);
    static string FormatTimeToStr(uint64 unixTime);
    static uint64 GetTimeNowUS();

private:
    Timer m_timer;
};