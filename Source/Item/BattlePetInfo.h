#pragma once

#include "../Precompiled.h"
#include "ItemUtils.h"

enum eBattlePetAbility
{
    PET_ABILITY_ATTACK,
    PET_ABILITY_HEAL_SELF,
    PET_ABILITY_REVIVE
};

string GetFullBattlePetName(int32 basePet, int32 pet2, int32 pet3);

class BattlePetInfo {
public:
    BattlePetInfo();
    ~BattlePetInfo();

public:
    int32 itemID = 0;
    string name;
    string subName;
    string endName;
    string powerName;
    uint8 element = ITEM_ELEMENT_NONE;
    uint8 cooldownSec = 0;
    uint8 ability = 0;
    uint8 abilityVal = 0;
    uint8 superAbility = 0;
    uint8 superAbilityVal = 0;
    uint8 superAbilityDurationSec = 0;
    int32 powerParticle = 0;
    int32 hitParticle = 0;
    string powerSound;
    string hitSound;
    string description;

public:
    string GetColorCodeByElement();
    string GetElementName();
    string GetDescribedPower();
};