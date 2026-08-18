#pragma once

#include "CrashHandler.h"

bool InitializeCrashLogFile(const char* appName, uint16 appID = 0);
void CloseCrashLogFile();
void SetCrashStatsBuffer(const char* formattedStats);
void DefaultCrashCallback(const CrashInfo& info);