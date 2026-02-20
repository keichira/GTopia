#pragma once

#include "../Precompiled.h"

static uint64 sRandSeed = 1;
static uint64 sRandIncrement = 1;
static uint64 sRandState = 1;

// Randomizer based on PCG32
// Used SplitMix64 for randomizing seed

uint32 RandomNext();
float RandomNextFloat();
void SetRandomSeed(uint64 seed);
void RandomizeRandomSeed();
uint32 rotr32(uint32 x, uint32 r);

int32 RandomRangeInt(int32 min, int32 max);
float RandomRangeFloat(float min, float max);