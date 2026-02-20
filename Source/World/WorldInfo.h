#pragma once

#include "../Precompiled.h"
#include "WorldTileManager.h"
#include "../Memory/MemoryBuffer.h"

enum eWorldGenerationType
{
    WORLD_GENERATION_DEFAULT = 0
};

enum eWeatherTypes 
{
    WEATHER_TYPE_DEFAULT = 0,
    WEATHER_TYPE_SUNSET = 1,
    WEATHER_TYPE_NIGHT = 2,
    WEATHER_TYPE_DESERT = 3,
    WEATHER_TYPE_SUNNY = 4,
    WEATHER_TYPE_RAINY_CITY = 5,
    WEATHER_TYPE_HARVEST = 6
};

class WorldInfo {
public:
    WorldInfo();
    virtual ~WorldInfo();

public:
    bool Serialize(MemoryBuffer& memBuffer, bool write, bool database);
    void GenerateWorld(eWorldGenerationType type, const Vector2Int& worldSize = {0, 0});

private:
    uint16 m_version;
    uint32 m_flags;
    string m_name;

    WorldTileManager* m_pTileMgr;

    uint32 m_defaultWeather;
    uint32 m_currentWeather;
};