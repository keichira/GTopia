#include "NetEntity.h"

int32 NetEntity::s_netID = NET_ID_RESERVED_UNTIL;

NetEntity::NetEntity()
: m_netID(s_netID++)
{
}

NetEntity::NetEntity(int32 id)
{
    m_netID = id;
}

NetEntity::~NetEntity()
{
}

void NetEntity::OnHandleDatabase(QueryTaskResult&&)
{
}

void NetEntity::OnHandleTCP(VariantVector&&)
{
}
