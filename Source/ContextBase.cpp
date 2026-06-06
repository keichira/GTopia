#include "ContextBase.h"

NetBurstConfig gNetBurstConfig;

void EvaluateNetHealth(usize queueSize, uint32 currentCpuPermille, uint32& outBurstLimit, bool& outIsPanic)
{
    auto& th = gNetBurstConfig.threshold;

    if(queueSize >= th.panicQueueSize || currentCpuPermille >= th.panicCpuPermille)
    {
        outBurstLimit = gNetBurstConfig.panicBurst;
        outIsPanic = true;
        return;
    }

    if(queueSize >= th.heavyQueueSize || currentCpuPermille >= th.heavyCpuPermille)
    {
        outBurstLimit = gNetBurstConfig.heavyBurst;
        outIsPanic = false;
        return;
    }

    outBurstLimit = gNetBurstConfig.normalBurst;
    outIsPanic = false;
}

ContextBase::ContextBase()
: m_stopFlag(0), m_shutdownFlag(0)
{
}

ContextBase::~ContextBase()
{
}

void ContextBase::Init()
{
}

void ContextBase::Kill()
{
}
