#include "NetEntity.h"

NetEntity::NetEntity()
: m_netID(sNetID++)
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
