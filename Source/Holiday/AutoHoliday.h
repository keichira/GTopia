#pragma once

#include "../Precompiled.h"

enum eAutoHoliday
{
    AUTO_HOLIDAY_COMET,

    AUTO_HOLIDAY_COUNT
};

class AutoHoliday {
public:
    AutoHoliday();
    virtual ~AutoHoliday();

public:
    virtual string GetName() const;
    virtual void OnStarted();
    virtual void OnEnded();
    virtual string GetWelcomeMessage() const;

public:
    bool IsActive() const { return m_active; }

protected:
    bool m_active;
};