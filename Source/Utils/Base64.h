#pragma once

#include "../Precompiled.h"

bool IsValidBase64Char(char c);
bool Base64_Decode(void* pData, uint32 size, string& out);