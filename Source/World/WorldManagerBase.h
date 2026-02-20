#pragma once

#include "../Precompiled.h"
#include "../Network/NetEntity.h"

class WorldManagerBase : public NetEntity {
public:
    WorldManagerBase() : NetEntity(NET_ID_WORLD_MANAGER) {}
};