#pragma once

#include "Precompiled.h"

class TileInfo;
class World;

struct HarmonicCrystalRecipe
{
    string crystals;
    int32 rewardID = 0;
    uint8 rewardCount = 0;
};

class HarmonicCrystal {
public:
    HarmonicCrystal();

public:
    HarmonicCrystalRecipe* GetRecipe(const string& crystals);
    char GetCrystalCodeFromID(int32 itemID);
    int16 GetChiAccuracy(TileInfo* pTile = nullptr, World* pWorld = nullptr);

private:
    uint16 EncodeRecipe(const string& crystals);
    void AddRecipe(const HarmonicCrystalRecipe& recipe);

private:
    std::unordered_map<uint16, HarmonicCrystalRecipe> m_recipes;
};

extern HarmonicCrystal gHarmonicCrystal;