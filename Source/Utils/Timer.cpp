#include "Timer.h"
#include <ctime>

Timer::Timer()
: m_startTime(GetTick())
{
}

void Timer::Reset()
{
    m_startTime = GetTick();
}

uint64 Timer::GetElapsedTime(bool reset)
{
    uint64 elapsedTime = GetTick() - m_startTime;

    if(reset) {
        Reset();
    }

    return elapsedTime;
}

uint64 Timer::GetStartTime() const
{
    return m_startTime;
}

Time::Time()
{
}

uint64 Time::GetSystemTime()
{
    return GetTick();
}

uint64 Time::GetTimeSinceEpoch()
{
    return time(NULL);
}

string Time::GetDateTimeStr()
{
    return GetDateTimeAsStr();
}
