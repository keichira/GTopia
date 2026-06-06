#pragma once

#include "Precompiled.h"
#include "Utils/Timer.h"
#include <csignal>

struct ContextPerfStats
{
    uint32 avgTickMs = 0;
    uint32 maxTickMs = 0;
    uint32 cpuPermille = 0;
    uint32 lagSpikeMs = 0;

    uint32 netCpuPermille = 0;
};

struct NetThreshold
{
    uint32 heavyQueueSize = 0;
    uint32 panicQueueSize = 0;

    uint32 heavyCpuPermille = 0;
    uint32 panicCpuPermille = 0;
};

struct NetBurstConfig
{
    NetThreshold threshold;

    uint32 normalBurst = 0;
    uint32 heavyBurst = 0;
    uint32 panicBurst = 0;

    uint32 maxPacketsPerSecond = 0;
};

extern NetBurstConfig gNetBurstConfig;

void EvaluateNetHealth(usize queueSize, uint32 currentCpuPermille, uint32& outBurstLimit, bool& outIsPanic);

class ContextBase {
public:
    ContextBase();
    virtual ~ContextBase();

public:
    virtual void Init();
    virtual void Kill();

public:
    void SetID(uint16 id) { m_id = id; }
    uint16 GetID() const { return m_id; }

    int32 IsRunning() { return m_stopFlag == 0; };
    int32 IsShutting() { return m_shutdownFlag == 1; }

    void Stop() { m_stopFlag = 1; }
    void Shutdown() { m_shutdownFlag = 1; }

    volatile sig_atomic_t* GetStopFlag() { return &m_stopFlag; }
    volatile sig_atomic_t* GetShutdownFlag() { return &m_shutdownFlag; }

    ContextPerfStats& GetPerfStats() { return m_perfStats; }

protected:
    uint16 m_id;
    volatile sig_atomic_t m_stopFlag;
    volatile sig_atomic_t m_shutdownFlag;
    ContextPerfStats m_perfStats;
};