#include "Role.h"
#include "../Utils/StringUtils.h"
#include "RoleManager.h"
#include "../IO/Log.h"

Role::Role()
: m_state(ROLE_RESOLVE_NONE), m_chatColor(0), m_nameColor(0)
{
}

void Role::AddPerm(uint32 permHash)
{
    m_basePerms.push_back(permHash);
}

void Role::FinalizePermissions()
{
    std::sort(m_finalPerms.begin(), m_finalPerms.end());

    auto it = std::unique(m_finalPerms.begin(), m_finalPerms.end());
    m_finalPerms.erase(it, m_finalPerms.end());
    
    m_finalPerms.shrink_to_fit();
}

bool Role::HasPerm(uint32 permHash) const
{
    return std::binary_search(m_finalPerms.begin(), m_finalPerms.end(), permHash);
}