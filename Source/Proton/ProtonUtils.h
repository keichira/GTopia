#pragma once

#include "../Precompiled.h"
#include "../Utils/Variant.h"

string XorCipherString(const string& str, const char* secret, int32 id);

namespace Proton
{
    uint8 ConvertVariantType(eVariantTypes type);
}
