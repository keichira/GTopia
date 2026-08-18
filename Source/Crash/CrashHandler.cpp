#include "CrashHandler.h"
#include "CrashReport.h"

static CrashCallback gCrashCallback = nullptr;

bool CrashHandler::Initialize(CrashCallback callback)
{
    gCrashCallback = callback ? callback : DefaultCrashCallback;
    return InitializeCrashHandler();
}

CrashCallback GetCrashCallback()
{
    return gCrashCallback;
}