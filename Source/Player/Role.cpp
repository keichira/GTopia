#include "Role.h"
#include "../Utils/StringUtils.h"
#include "RoleManager.h"
#include "../IO/Log.h"

Role::Role()
: m_state(ROLE_RESOLVE_NONE)
{
}

void Role::AddPerm(eRolePerm perm)
{
    uint32 permIdx = static_cast<uint32>(perm);
    uint32 blockIdx = permIdx / 32;
    uint32 bitIdx = permIdx % 32;

    if(blockIdx >= m_basePerms.size()) {
        m_basePerms.resize(blockIdx + 1, 0);
    }

    m_basePerms[blockIdx] |= (1 << bitIdx);
}

bool Role::HasPerm(eRolePerm perm)
{
    uint32 permIdx = static_cast<uint32>(perm);
    uint32 blockIdx = permIdx / 32;
    uint32 bitIdx = permIdx % 32;

    if(blockIdx >= m_finalPerms.size()) {
        return false;
    }

    return (m_finalPerms[blockIdx] & (1 << bitIdx));
}
