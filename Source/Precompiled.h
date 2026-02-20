#pragma once

#include "OS/OSPrecompiled.h"

#include <unordered_map>
#include <vector>
#include <cstring>

#ifndef SAFE_DELETE
#define SAFE_DELETE(p) { if(p) { delete (p); (p) = nullptr; } }
#endif

#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p); (p) = nullptr; } }
#endif

#ifndef SAFE_FREE
#define SAFE_FREE(p) { if(p) { free(p); (p) = nullptr; } }
#endif