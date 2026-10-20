#include "Timer.h"
#include "StringUtils.h"
#include <chrono>
#include <ctime>

Timer::Timer() : m_startTime(GetTick()), m_isSet(false) {}

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
    m_isSet = true;
}

void Timer::UnSet()
{
    m_isSet = false;
}

bool Timer::IsPassed()
{
    if (!m_isSet)
        return false;

    if (GetTick() >= m_startTime)
    {
        m_isSet = false;
        return true;
    }

    return false;
}

uint64 Timer::GetRemainingTime()
{
    uint64 now = GetTick();

    if (now >= m_startTime)
        return 0;

    return m_startTime - now;
}

uint64 Timer::GetElapsedTime(bool reset)
{
    uint64 now = GetTick();

    uint64 elapsedTime = (now > m_startTime) ? (now - m_startTime) : 0;

    if (reset)
    {
        Reset();
    }

    return elapsedTime;
}

uint64 Timer::GetStartTime() const
{
    return m_startTime;
}

Time::Time() {}

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

    if (days > 0)
        ret += ToString(days) + " days ";
    if (hours > 0)
        ret += ToString(hours) + " hours ";
    if (minutes > 0)
        ret += ToString(minutes) + " mins ";
    if (seconds > 0)
        ret += ToString(seconds) + " secs ";

    if (!ret.empty() && ret.back() == ' ')
        ret.pop_back();

    return ret;
}

int32 Time::GetDaysInMonth(int32 year, int32 month)
{
    if (month == 2)
    {
        bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        return isLeap ? 29 : 28;
    }
    if (month == 4 || month == 6 || month == 9 || month == 11)
        return 30;

    return 31;
}

string Time::FormatTimeToStr(time_t targetTime)
{
    int32 m = 0, d = 0, y = 0, h = 0, min = 0, s = 0;
    GetTimeLocal(&targetTime, &m, &d, &y, &h, &min, &s);

    char buf[64];
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d", y, m, d, h, min, s);
    return string(buf);
}

string Time::FormatTimeToStr(uint64 unixTime)
{
    return FormatTimeToStr((time_t)(unixTime));
}

uint64 Time::GetTimeNowUS()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}
