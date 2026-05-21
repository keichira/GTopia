#pragma once

// linear based

#include "../Precompiled.h"

class WeightRand {
public:
    struct Element
    {
        int32 num;
        int32 weight;
    };

public:
    WeightRand();
    ~WeightRand();

public:
    void Add(int32 num, int32 weight);
    int32 GetTotalWeight();
    bool Roll(int32& out);

private:
    std::vector<Element> m_elements;
    bool m_changed;
    int32 m_lastWeight;
};