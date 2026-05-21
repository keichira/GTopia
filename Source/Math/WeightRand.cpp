#include "WeightRand.h"
#include "Random.h"

WeightRand::WeightRand()
: m_changed(true), m_lastWeight(0)
{
}

WeightRand::~WeightRand()
{
}

void WeightRand::Add(int32 num, int32 weight)
{
    if(weight <= 0)
        return;

    m_changed = true;
    m_elements.push_back({ num, weight });
}

int32 WeightRand::GetTotalWeight()
{
    if(!m_changed)
        return m_lastWeight;

    int32 total = 0;
    for(auto& elem : m_elements)
    {
        total += elem.weight;
    }

    m_lastWeight = total;
    m_changed = false;

    return total;
}

bool WeightRand::Roll(int32& out)
{
    if(m_elements.empty())
        return false;

    int32 total = GetTotalWeight();
    if(total <= 0)
        return false;

    uint32 r = RandomNext();
    int32 t = (int32)((uint64)r * (uint64)total >> 32);

    int32 acc = 0;

    for(auto& elem : m_elements)
    {
        acc += elem.weight;
        if(t < acc)
        {
            out = elem.num;
            return true;
        }
    }

    return false;
}
