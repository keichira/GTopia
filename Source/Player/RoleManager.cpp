#include "RoleManager.h"
#include "../Utils/StringUtils.h"
#include "../IO/Log.h"
#include "../Utils/ConfigDB.h"

RoleManager::RoleManager() :
m_defaultRoleID(-1)
{
}

RoleManager::~RoleManager()
{
    Kill();
}

bool RoleManager::Load(const string& filePath)
{
    ConfigDB cfg;
    if(!cfg.Load(filePath))
        return false;

    Role* pLastRole = nullptr;

    for(auto& line : cfg.Lines())
    {
        const string& key = line.GetString(0);

        if(key == "set_default_role")
        {
            if(!line.Require(1))
                return false;

            m_defaultRoleID = line.GetUInt(1);

            if(m_defaultRoleID < 1)
            {
                LOGGER_LOG_ERROR("Did you forgot to set default role in roles.txt? it should bigger than 0");
                return false;
            }
        }

        if(key == "add_role")
        {
            if(!line.Require(4))
                return false;

            Role* pRole = new Role();
            
            pRole->m_name = line.GetString(1);
            pRole->m_id = line.GetUInt(2);
            pRole->m_prefix = line.GetString(3);
            pRole->m_suffix = line.GetString(4);

            pRole->m_finalPerms.push_back(0);

            pLastRole = pRole;
            m_roles.insert_or_assign(pRole->GetID(), pRole);
        }

        if(key == "set_inherits")
        {
            if(!pLastRole)
                continue;

            for(uint8 i = 1; i < line.GetArgSize(); ++i)
            {
                pLastRole->m_inherits.push_back(line.GetInt(i));
            }
        }

        if(key == "set_name_color")
        {
            if(!pLastRole)
                continue;

            if(!line.GetString(1).empty())
                pLastRole->m_nameColor = line.GetString(1)[0];
        }

        if(key == "set_chat_color")
        {
            if(!pLastRole)
                continue;

            if(!line.GetString(1).empty())
                pLastRole->m_chatColor = line.GetString(1)[0];
        }

        if(key == "set_perms")
        {
            if(!pLastRole)
                continue;

            for(uint32 i = 1; i < line.GetArgSize(); ++i)
            {
                string permStr = line.GetString(i);
                if(permStr.empty()) 
                    continue;

                pLastRole->AddPerm(HashString(permStr));
            }
        }
    }

    for(auto& [_, pRole] : m_roles) {
        if(pRole->m_state == ROLE_RESOLVE_NONE) {
            ResolveRole(pRole);
        }
    }

    return true;
}

bool RoleManager::ResolveRole(Role* pRole)
{
    if(!pRole) 
        return false;

    if(pRole->m_state == ROLE_RESOLVE_DONE) 
        return true;

    if(pRole->m_state == ROLE_RESOLVE_PROCESSING) 
        return false;

    pRole->m_state = ROLE_RESOLVE_PROCESSING;
    pRole->m_finalPerms = pRole->m_basePerms;

    for(uint32 parentID : pRole->m_inherits) 
    {
        Role* pParent = GetRole(parentID);
        if(!pParent) 
            continue;

        if(!ResolveRole(pParent)) 
            return false;

        pRole->m_finalPerms.insert(pRole->m_finalPerms.end(), pParent->m_finalPerms.begin(), pParent->m_finalPerms.end());
    }

    pRole->FinalizePermissions();
    pRole->m_state = ROLE_RESOLVE_DONE;
    return true;
}

void RoleManager::Kill()
{
    for(auto& [_, pRole] : m_roles) {
        SAFE_DELETE(pRole);
    }

    m_roles.clear();
}

Role* RoleManager::GetRole(int32 id)
{
    auto it = m_roles.find(id);
    if(it == m_roles.end()) {
        return nullptr;
    }

    return it->second;
}

RoleManager* GetRoleManager() { return RoleManager::GetInstance(); }
