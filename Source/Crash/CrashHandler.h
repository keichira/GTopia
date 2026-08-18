#pragma once

#include "../Precompiled.h"

struct CrashInfo
{
    uint32 code = 0;
    uintptr_t address = 0;

    const char* reason = nullptr;
    const char* message = nullptr;
};

using CrashCallback = void (*)(const CrashInfo& info);

class CrashHandler
{
public:
    static bool Initialize(CrashCallback callback = nullptr);
};

CrashCallback GetCrashCallback();

bool InitializeCrashHandler();