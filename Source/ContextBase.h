#pragma once

#include "Precompiled.h"
#include <csignal>

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
    void Stop() { m_stopFlag = 1; }

protected:
    uint16 m_id;
    volatile sig_atomic_t m_stopFlag;
};