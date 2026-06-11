#include "WorldInfo.h"
#include "../Utils/StringUtils.h"
#include "../Math/Random.h"

bool IsValidWorldName(const string& worldName)
{
    const char* src = worldName.c_str();

    while(*src) {
        if((IsAlpha(*src) && IsUpper(*src)) || IsDigit(*src)) {
            src++;
            continue;
        }

        return false;
    }

    return true;
}

WorldInfo::WorldInfo()
: m_version(14), m_flags(0), m_defaultWeather(0), m_currentWeather(0)
{
    m_pTileMgr = new WorldTileManager();
    m_pObjMgr = new WorldObjectManager();
}

WorldInfo::~WorldInfo()
{
    Kill();
}

void WorldInfo::Kill()
{
    SAFE_DELETE(m_pTileMgr);
    SAFE_DELETE(m_pObjMgr);
}

bool WorldInfo::Serialize(MemoryBuffer& memBuffer, bool write, bool database, float gameVersion)
{
    memBuffer.ReadWrite(m_version, write);
    memBuffer.ReadWrite(m_flags, write);
    memBuffer.ReadWriteString(m_name, write);

    if(!m_pTileMgr->Serialize(memBuffer, write, database, this, gameVersion)) {
        return false;
    }

    if(!database && write && gameVersion >= 5.40f) // 5.40 is not the actual value lazy to dig for it
    {
        uint32 unk = 0;
        memBuffer.ReadWrite(unk, write);
        memBuffer.ReadWrite(unk, write);
        memBuffer.ReadWrite(unk, write);
    }

    m_pObjMgr->Serialize(memBuffer, write, database);
    
    uint16 unused = 0;
    memBuffer.ReadWrite(m_defaultWeather, write);
    memBuffer.ReadWrite(unused, write);
    memBuffer.ReadWrite(m_currentWeather, write);
    memBuffer.ReadWrite(unused, write);
    memBuffer.ReadWrite(unused, write);
    return true;
}

void WorldInfo::GenerateWorld(eWorldGenerationType type)
{

    switch(type) {
        case WORLD_GENERATION_DEFAULT: {
            m_defaultWeather = WEATHER_TYPE_DEFAULT;
            m_currentWeather = WEATHER_TYPE_DEFAULT;

            m_pTileMgr->GenerateDefaultMap();
            break;
        }

        case WORLD_GENERATION_CLEAR: {
            m_defaultWeather = WEATHER_TYPE_DEFAULT;
            m_currentWeather = WEATHER_TYPE_DEFAULT;

            m_pTileMgr->GenerateClearMap();
            break;
        }
    }
}

uint32 WorldInfo::GetMemEstimate(bool database, float gameVersion)
{
    uint32 memSize = 0;
    memSize += sizeof(uint16) + sizeof(m_flags) + 2 + m_name.size();
    memSize += m_pTileMgr->GetMemEstimate(database, this, gameVersion);
    memSize += m_pObjMgr->GetMemEstimate();
    memSize += sizeof(m_defaultWeather) + sizeof(m_currentWeather);

    return memSize;
}
