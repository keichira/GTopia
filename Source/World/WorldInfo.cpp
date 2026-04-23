#include "WorldInfo.h"

WorldInfo::WorldInfo()
: m_version(10), m_flags(0)
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

bool WorldInfo::Serialize(MemoryBuffer &memBuffer, bool write, bool database)
{
    memBuffer.ReadWrite(m_version, write);
    memBuffer.ReadWrite(m_flags, write);
    memBuffer.ReadWriteString(m_name, write);

    if(!m_pTileMgr->Serialize(memBuffer, write, database, this)) {
        return false;
    }

    m_pObjMgr->Serialize(memBuffer, write);
    
    memBuffer.ReadWrite(m_defaultWeather, write);
    memBuffer.ReadWrite(m_currentWeather, write);
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

uint32 WorldInfo::GetMemEstimate(bool database)
{
    uint32 memSize = 0;
    memSize += sizeof(m_version) + sizeof(m_flags) + 2 + m_name.size();
    memSize += m_pTileMgr->GetMemEstimate(database, this);
    memSize += m_pObjMgr->GetMemEstimate();
    memSize += sizeof(m_defaultWeather) + sizeof(m_currentWeather);

    return memSize;
}
