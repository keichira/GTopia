#pragma once

#include "../Precompiled.h"

static uint64 s_netID = 0;

enum eEntityType
{
    ENTITY_TYPE_SYSTEM,
    ENTITY_TYPE_PLAYER,
    ENTITY_TYPE_WORLD,
    ENTITY_TYPE_GUILD
};

class NetEntity {
public:
    NetEntity(eEntityType type);

public:
    static eEntityType GetType(uint64 id) { return eEntityType(id >> 56); }

public:
    uint32 GetNetID() const { return uint32(m_entityID & 0xFFFFFFFFULL); }
    eEntityType GetType() const { return GetType(m_entityID); }
    uint64 GetEntityID() const { return m_entityID; }

private:
    uint64 m_entityID;
};