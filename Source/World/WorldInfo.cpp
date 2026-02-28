#include "WorldInfo.h"

WorldInfo::WorldInfo()
: m_version(10), m_flags(0)
{
    m_pTileMgr = new WorldTileManager();
    m_pObjMgr = new WorldObjectManager();
}

WorldInfo::~WorldInfo()
{
}

bool WorldInfo::Serialize(MemoryBuffer& memBuffer, bool write, bool database)
{
    memBuffer.ReadWrite(m_version, write);
    memBuffer.ReadWrite(m_flags, write);
    memBuffer.ReadWriteString(m_name, write);

    if(!m_pTileMgr->Serialize(memBuffer, write, database)) {
        return false;
    }

    m_pObjMgr->Serialize(memBuffer, write);
    
    memBuffer.ReadWrite(m_defaultWeather, write);
    memBuffer.ReadWrite(m_currentWeather, write);
    return true;
}

void WorldInfo::GenerateWorld(eWorldGenerationType type, const Vector2Int& worldSize)
{    
    if(worldSize.x != 0 && worldSize.y != 0) {
        m_pTileMgr->SetSize(worldSize);
    }

    switch(type) {
        case WORLD_GENERATION_DEFAULT: {
            m_defaultWeather = WEATHER_TYPE_DESERT;
            m_currentWeather = WEATHER_TYPE_DESERT;

            m_pTileMgr->GenerateDefaultMap();
            break;
        }
    }
}
