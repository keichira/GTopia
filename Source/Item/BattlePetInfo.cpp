#include "BattlePetInfo.h"
#include "../Utils/StringUtils.h"

BattlePetInfo::BattlePetInfo()
{
}

BattlePetInfo::~BattlePetInfo()
{
}

string BattlePetInfo::GetColorCodeByElement()
{
    switch(element)
    {
        case ITEM_ELEMENT_EARTH:
            return "`2";

        case ITEM_ELEMENT_FIRE:
            return "`4";

        case ITEM_ELEMENT_AIR:
            return "`9";

        case ITEM_ELEMENT_WATER:
            return "`1";

        default:
            return "";
    }
}

string BattlePetInfo::GetElementName()
{
    switch(element)
    {
        case ITEM_ELEMENT_EARTH:
            return "Earth";

        case ITEM_ELEMENT_FIRE:
            return "Fire";

        case ITEM_ELEMENT_AIR:
            return "Water";

        case ITEM_ELEMENT_WATER:
            return "Water";

        default:
            return "";
    }
}

string BattlePetInfo::GetDescribedPower()
{
    string out = GetColorCodeByElement() + powerName + ":`` " + description;
    
    if(abilityVal > 0)
    {
        switch(ability)
        {
            case PET_ABILITY_HEAL_SELF:
                out += " Heals " + ToString(abilityVal) + " life.";
                break;

            case PET_ABILITY_REVIVE:
                out += " Heals " + ToString(abilityVal) + "% life";
                break;

            default:
                out += "Inflicts " + ToString(abilityVal) + " " + GetElementName() + " damage.";
        }
    }

    if(cooldownSec > 0)
    {
        out += " Cooldown: " + ToString(cooldownSec);
    }

    return out;
}
