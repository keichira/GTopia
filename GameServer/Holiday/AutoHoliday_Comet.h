#pragma once

#include "Precompiled.h"
#include "Holiday/AutoHoliday.h"

class AutoHoliday_Comet : public AutoHoliday {
public:
    AutoHoliday_Comet();
    ~AutoHoliday_Comet();

private:
    string GetName() const override;
    void OnStarted() override;
    void OnEnded() override;
};