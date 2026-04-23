#include "NetEntity.h"

NetEntity::NetEntity(eEntityType type)
{
    m_entityID = (uint64(type) << 56) | (++s_netID & 0xFFFFFFFFULL);
}