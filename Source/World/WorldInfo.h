#pragma once

#include "../Precompiled.h"
#include "WorldTileManager.h"
#include "../Memory/MemoryBuffer.h"
#include "WorldObjectManager.h"

enum eWorldGenerationType
{
    WORLD_GENERATION_DEFAULT = 0,
    WORLD_GENERATION_CLEAR = 1
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
    void Kill();

    bool Serialize(MemoryBuffer& memBuffer, bool write, bool database);
    void GenerateWorld(eWorldGenerationType type);
    uint32 GetMemEstimate(bool database);

    void SetName(const string& worldName) { m_name = worldName; }
    const string& GetWorlName() const { return m_name; } 

    void SetCurrentWeather(uint32 newWeather) { m_currentWeather = newWeather; }
    uint32 GetCurrentWeather() const { return m_currentWeather; }

    uint32 GetDefaultWeather() const { return m_defaultWeather; }

    uint16 GetWorldVersion() const { return m_version; }

    WorldTileManager* GetTileManager() { return m_pTileMgr; };
    WorldObjectManager* GetObjectManager() { return m_pObjMgr; }

private:
    uint16 m_version;
    uint32 m_flags;
    string m_name;

    WorldTileManager* m_pTileMgr;
    WorldObjectManager* m_pObjMgr;

    uint32 m_defaultWeather;
    uint32 m_currentWeather;
};