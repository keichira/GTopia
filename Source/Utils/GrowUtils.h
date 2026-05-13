#pragma once

#include "../Precompiled.h"
#include "../Math/Vector2.h"

static uint32 sGemTypeVec[5] = { 1, 5, 10, 50, 100 };

Vector2Float GetRandomItemDropOffset();
Vector2Float GetRandomPlayerItemDropOffset();
std::vector<int32> ParseGemIntoGemTypes(int32 gemCount);
uint32 GetGemAmountByGemType(uint32 gemType);