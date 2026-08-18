#include "ServerRuntimeStats.h"
#include "../Utils/Timer.h"

void ServerRuntimeStats::Init()
{
    m_startTime = Time::GetTimeSinceEpoch();
}

uint64 ServerRuntimeStats::GetUptimeSeconds() const
{
    return Time::GetTimeSinceEpoch() - m_startTime;
}