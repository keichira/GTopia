#pragma once

#include "../Precompiled.h"
#include "../Math/Vector2.h"
#include "../Memory/MemoryBuffer.h"

struct WorldObject
{
    uint32 objectID;
    uint16 itemID;
    uint8 count;
    uint8 flags;
    uint32 indexID;
    Vector2Float pos;

    void Serialize(MemoryBuffer& memBuffer, bool write);
};

class WorldObjectManager {
public:
    WorldObjectManager();
    ~WorldObjectManager();

public:
    void Serialize(MemoryBuffer& memBuffer, bool write);

private:
    std::vector<WorldObject> m_objects;
    uint32 m_objectCount;
    uint32 m_lastIndex;
};