#pragma once

#include "../Precompiled.h"
#include "../Math/Vector2.h"

static uint32 sGemTypeVec[5] = { 1, 5, 10, 50, 100 };

struct OuijaBoardCloth
{
    int32 bodyPart = 0;
    int32 itemID = 0;
};

static std::vector<OuijaBoardCloth> gOuijaSpiritCloth;
static std::vector<OuijaBoardCloth> gOuijaDarkSpiritCloth;

Vector2Float GetRandomItemDropOffset();
Vector2Float GetRandomPlayerItemDropOffset();
std::vector<int32> ParseGemIntoGemTypes(int32 gemCount);
uint32 GetGemAmountByGemType(uint32 gemType);

string GetRandomGrowNamePart();

std::vector<OuijaBoardCloth> GetOuijaBoardCloth(bool isDarkSpiritBoard);