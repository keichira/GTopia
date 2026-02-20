#pragma once

#include "../Precompiled.h"
#include "../Utils/Variant.h"
#include "../Database/QueryUtils.h"

#define NET_ID_RESERVED_UNTIL 6
#define NET_ID_FALLBACK 0
#define NET_ID_WORLD_MANAGER 1

static int32 sNetID = NET_ID_RESERVED_UNTIL;

class NetEntity {
public:
    NetEntity();
    NetEntity(int32 id);
    virtual ~NetEntity();

    int32 GetNetID() const { return m_netID; }
    
    virtual void OnHandleDatabase(QueryTaskResult&&);
    virtual void OnHandleTCP(VariantVector&&);

private:
    int32 m_netID;
};