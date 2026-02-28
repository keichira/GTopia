#include "WorldObjectManager.h"

void WorldObject::Serialize(MemoryBuffer& memBuffer, bool write)
{
    memBuffer.ReadWrite(itemID, write);
    memBuffer.ReadWrite(pos, write);
    memBuffer.ReadWrite(count, write);
    memBuffer.ReadWrite(flags, write);
    memBuffer.ReadWrite(objectID, write);
}

WorldObjectManager::WorldObjectManager()
: m_objectCount(0), m_lastIndex(0)
{
}

WorldObjectManager::~WorldObjectManager()
{
}

void WorldObjectManager::Serialize(MemoryBuffer& memBuffer, bool write)
{
    memBuffer.ReadWrite(m_objectCount, write);
    memBuffer.ReadWrite(m_lastIndex, write);

    if(!write) {
        m_objects.resize(m_objectCount);
    }

    for(auto& obj : m_objects) {
        obj.Serialize(memBuffer, write);
    }
}
