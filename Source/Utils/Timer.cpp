#include "Timer.h"
#include <ctime>
#include "StringUtils.h"

Timer::Timer()
: m_startTime(GetTick())
{
}

void Timer::Reset()
{
    m_startTime = GetTick();
}

void Timer::Reset(uint64 newTime)
{
    m_startTime = newTime;
}

void Timer::Set(uint64 ms)
{
    m_startTime = GetTick() + ms;
}

bool Timer::IsPassed()
{
    return GetTick() >= m_startTime;
}

uint64 Timer::GetRemainingTime()
{
    uint64 now = GetTick();

    if(now >= m_startTime)
        return 0;

    return m_startTime - now;
}

uint64 Timer::GetElapsedTime(bool reset)
{
    uint64 now = GetTick();

    uint64 elapsedTime = (now > m_startTime) ? (now - m_startTime) : 0;

    if(reset)
    {
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

uint32 Time::GetSecondsFromMidnight()
{
    return ::GetSecondsFromMidnight();
}

string Time::GetDateTimeStr()
{
    return GetDateTimeAsStr();
}

string Time::ConvertTimeToStr(uint64 ms)
{
    int32 days = ms / 86400000;
    ms %= 86400000;

    int32 hours = ms / 3600000;
    ms %= 3600000;

    int32 minutes = ms / 60000;
    ms %= 60000;

    int32 seconds = ms / 1000;
    ms %= 1000;

    string ret;

    if(days > 0) ret += ToString(days) + " days ";
    if(hours > 0) ret += ToString(hours) + " hours ";
    if(minutes > 0) ret += ToString(minutes) + " mins ";
    if(seconds > 0) ret += ToString(seconds) + " secs ";

    return ret;
}
