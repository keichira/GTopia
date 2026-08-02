#include "IPTracker.h"

IPTracker::IPTracker() {}

IPTracker::~IPTracker() {}

bool IPTracker::IsBanned(uint32 ip)
{
    auto it = m_tracks.find(ip);
    if (it == m_tracks.end())
        return false;

    return !it->second.bannedUntil.IsPassed();
}

bool IPTracker::CheckCooldown(uint32 ip, uint32 cooldownSec)
{
    if (IsBanned(ip))
        return false;

    auto& info = m_tracks[ip];

    if ((info.lastSeenTimer.GetElapsedTime(false) / 1000) < cooldownSec)
        return false;

    info.lastSeenTimer.Reset();
    return true;
}

void IPTracker::RegisterFailure(uint32 ip, uint32 maxFails, uint32 banDurationSec)
{
    auto& info = m_tracks[ip];
    info.failCount++;

    if (info.failCount >= maxFails)
    {
        info.bannedUntil.Set(banDurationSec * 1000);
    }
}

void IPTracker::ResetFailures(uint32 ip)
{
    auto it = m_tracks.find(ip);
    if (it != m_tracks.end())
    {
        it->second.failCount = 0;
        it->second.bannedUntil.Reset(0);
    }
}

void IPTracker::Cleanup()
{
    for (auto it = m_tracks.begin(); it != m_tracks.end();)
    {
        if (it->second.bannedUntil.IsPassed() && it->second.lastSeenTimer.GetElapsedTime() > 300000)
        {
            it = m_tracks.erase(it);
        }
        else
        {
            ++it;
        }
    }
}