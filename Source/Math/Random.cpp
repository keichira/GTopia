#include "Random.h"

void SetRandomSeed(uint64 seed)
{
    sRandSeed = seed;
}

void RandomizeRandomSeed()
{
    uint64 t = sRandSeed + 0x9E3779B97F4A7C15ULL;
    t = (t ^ (t >> 30)) * 0xBF58476D1CE4E5B9ULL;
    t = (t ^ (t >> 27)) * 0x94D049BB133111EBULL;
    sRandState = t ^ (t >> 31);
    sRandIncrement = (sRandState << 1) | 1;
    RandomNext();
}

uint32 rotr32(uint32 x, uint32 r)
{
    return (x >> r) | (x << (-r & 31));
}

uint32 RandomNext()
{
    uint64 x = sRandState;
    unsigned count = (unsigned)(x >> 59);
    
    sRandState = x * 6322133225846793005u + sRandIncrement;
    x ^= x >> 18;
    
    return rotr32((uint32)(x >> 27), count);
}

float RandomNextFloat()
{
    return RandomNext() / (float)(0xFFFFFFFF);
}

int32 RandomRangeInt(int32 min, int32 max)
{
    uint32 r = RandomNext();
    uint64 range = (uint64)(max - min + 1);
    return min + (int32)((r * range) >> 32);
}

float RandomRangeFloat(float min, float max)
{
    return min + (max - min) * RandomNextFloat();
}