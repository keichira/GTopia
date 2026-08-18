#pragma once

#include "Crash/CrashContext.h"
#include "OS/OSPrecompiled.h"

#include <cstring>
#include <unordered_map>
#include <vector>

// clang-format off

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) { if(p) { delete (p); (p) = nullptr; } }
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p); (p) = nullptr; } }
#endif

#ifndef SAFE_FREE
#define SAFE_FREE(p) { if(p) { free(p); (p) = nullptr; } }
#endif