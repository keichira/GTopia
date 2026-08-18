#pragma once

#include "Precompiled.h"

struct ContextPerfStats
{
    uint32 avgTickMs = 0;
    uint32 maxTickMs = 0;
    uint32 cpuPermille = 0;
    uint32 lagSpikeMs = 0;

    uint32 netCpuPermille = 0;
};

class ServerRuntimeStats
{
public:
    void Init();

public:
    ContextPerfStats& GetPerfStats() { return m_perfStats; }
    uint64 GetUptimeSeconds() const;

private:
    ContextPerfStats m_perfStats;

    uint64 m_startTime;
};