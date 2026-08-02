#pragma once

#include "../Precompiled.h"
#include "../Utils/Timer.h"

class IPTracker
{
public:
    struct TrackerInfo
    {
        Timer lastSeenTimer;
        Timer bannedUntil;
        uint32 failCount = 0;
    };

public:
    IPTracker();
    ~IPTracker();

public:
    bool IsBanned(uint32 ip);
    bool CheckCooldown(uint32 ip, uint32 cooldownSec);
    void RegisterFailure(uint32 ip, uint32 maxFails, uint32 banDurationSec);
    void ResetFailures(uint32 ip);
    void Cleanup();

private:
    std::unordered_map<uint32, TrackerInfo> m_tracks;
};