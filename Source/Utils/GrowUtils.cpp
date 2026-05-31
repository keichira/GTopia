#include "GrowUtils.h"
#include "../Math/Random.h"
#include "../Item/ItemInfoManager.h"

Vector2Float GetRandomItemDropOffset()
{
    Vector2Float ret;
    ret.x = RandomRangeInt(-7, 6);
    ret.y = RandomRangeInt(-11, 10);

    return ret;
}

Vector2Float GetRandomPlayerItemDropOffset()
{
    Vector2Float ret;
    ret.x = RandomRangeInt(-7, 6);
    ret.y = RandomRangeInt(-3, 2);

    return ret;
}

std::vector<int32> ParseGemIntoGemTypes(int32 gemCount)
{
    std::vector<int32> out;

    while(gemCount > 0)
    {
        for(int32 i = 4; i >= 0; --i)
        {
            uint32 gemValue = sGemTypeVec[i];

            if(gemValue <= gemCount)
            {
                gemCount -= gemValue;
                out.push_back(i);
                break;;
            }
        }
    }

    return out;
}

uint32 GetGemAmountByGemType(uint32 gemType)
{
    if(gemType > 4)
        return sGemTypeVec[0];

    return sGemTypeVec[gemType];
}

string GetRandomGrowNamePart()
{
    switch(RandomRangeInt(0, 23))
    {
        case 0: return "Banana";
        case 1: return "Watch";
        case 2: return "Death";
        case 3: return "Wiggle";
        case 4: return "Board";
        case 5: return "Laugh";
        case 6: return "Smile";
        case 7: return "Azure";
        case 8: return "Squish";
        case 9: return "Punch";
        case 10: return "Brave";
        case 11: return "Krazy";
        case 12: return "Solid";
        case 13: return "Fairy";
        case 14: return "Snake";
        case 15: return "Tickle";
        case 16: return "Shiny";
        case 17: return "Bucks";
        case 18: return "Mouse";
        case 19: return "Smell";
        case 20: return "Sickle";
        case 21: return "Einst";
        case 22: return "Flash";
        default: return "Wiggle";
    }
}

std::vector<OuijaBoardCloth> GetOuijaBoardCloth(bool isDarkSpiritBoard)
{
    if(gOuijaSpiritCloth.empty() || gOuijaDarkSpiritCloth.empty())
    {
        gOuijaSpiritCloth.clear();
        gOuijaDarkSpiritCloth.clear();

        uint16 minSpiritRarity = 10;
        uint16 maxSpiritRarity = 40;

        uint16 minDarkSpiritRarity = 40;
        uint16 maxDarkSpiritRarity = 100;

        ItemInfoManager* pItemMgr = GetItemInfoManager();
        for(int32 i = 0; i < pItemMgr->GetItemCount(); ++i)
        {
            ItemInfo* pItem = GetItemInfoManager()->GetItemByID(i);
            if(!pItem)
                continue;

            if(pItem->type != ITEM_TYPE_CLOTHES || pItem->rarity < minSpiritRarity || pItem->rarity > maxDarkSpiritRarity ||
                pItem->HasFlag(ITEM_FLAG_HOLIDAY) || pItem->HasFlag(ITEM_FLAG_BETA) || pItem->HasFlag(ITEM_FLAG_MOD))
                continue;

            if(pItem->rarity >= minSpiritRarity && pItem->rarity < maxSpiritRarity)
            {
                gOuijaSpiritCloth.emplace_back(OuijaBoardCloth{pItem->bodyPart, pItem->id});
            }

            if(pItem->rarity >= minDarkSpiritRarity && pItem->rarity < maxDarkSpiritRarity)
            {
                gOuijaDarkSpiritCloth.emplace_back(OuijaBoardCloth{pItem->bodyPart, pItem->id});
            }
        }
    }

    auto& list = isDarkSpiritBoard ? gOuijaDarkSpiritCloth : gOuijaSpiritCloth;
    int32 lastBodyType = -1;

    std::vector<OuijaBoardCloth> out;
    int32 attempt = 0;

    while(attempt < 15 && out.size() != 2)
    {
        auto& chosenItem = list[RandomRangeInt(0, list.size() - 1)];
        if(lastBodyType == chosenItem.bodyPart)
        {
            attempt++;
            continue;
        }

        out.push_back(chosenItem);
    }

    return out;
}
