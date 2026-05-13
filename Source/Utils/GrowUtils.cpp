#include "GrowUtils.h"
#include "../Math/Random.h"

Vector2Float GetRandomItemDropOffset()
{
    Vector2Float ret;
    ret.x = RandomRangeInt(-8, 8);
    ret.y = RandomRangeInt(-12, 12);

    return ret;
}

Vector2Float GetRandomPlayerItemDropOffset()
{
    Vector2Float ret;
    ret.x = RandomRangeInt(-8, 8);
    ret.y = RandomRangeInt(-4, 4);

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
