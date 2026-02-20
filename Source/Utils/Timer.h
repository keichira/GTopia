#pragma once

#include "../Precompiled.h"

class Timer {
public:
    Timer();

public:
    void Reset();
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
    static string GetDateTimeStr();

private:
    Timer m_timer;
};