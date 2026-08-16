#pragma once

#include "Item/ItemInfoManager.h"
#include "Precompiled.h"

struct SuperHeroCard
{
    string name;
    int32 itemID = 0;
    uint8 element = ITEM_ELEMENT_NONE;
    int32 damage = 0;
};

struct SuperHeroVillian
{
    string name;
    string introText;
    string battleText;
    string defeatText;

    int32 cards[5] = {0};
    uint8 element = ITEM_ELEMENT_NONE;
};

std::vector<SuperHeroCard> g_superHeroCards;
std::vector<SuperHeroVillian> g_superHeroVillians;
