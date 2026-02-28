#include "RoleManager.h"
#include "../IO/File.h"
#include "../Utils/StringUtils.h"
#include "../IO/Log.h"

RoleManager::RoleManager()
{
    InitRolePerms();
}

RoleManager::~RoleManager()
{
    for(auto& [_, pRole] : m_roles) {
        SAFE_DELETE(pRole);
    }

    m_roles.clear();
}

bool RoleManager::Load(const string& filePath)
{
    File file;
    if(!file.Open(filePath)) {
        return false;
    }

    uint32 fileSize = file.GetSize();
    string fileData(fileSize, '\0');

    if(file.Read(fileData.data(), fileSize) != fileSize) {
        return false;
    }

    auto lines = Split(fileData, '\n');

    Role* pLastRole = nullptr;

    for(auto& line : lines) {
        if(line.empty() || line[0] == '#') {
            continue;
        }

        auto args = Split(line, '|');

        if(args[0] == "add_role") {
            Role* pRole = new Role();
            pRole->m_name = args[1];
            pRole->m_id = ToInt(args[2]);
            pRole->m_prefix = args[3];
            pRole->m_suffix = args[4];

            pLastRole = pRole;
            m_roles.insert_or_assign(pRole->GetID(), pRole);
        }

        if(args[0] == "set_inherits") {
            if(!pLastRole) {
                continue;
            }

            for(uint8 i = 1; i < args.size(); ++i) {
                pLastRole->m_inherits.push_back(ToInt(args[i]));
            }
        }

        if(args[0] == "set_name_color") {
            if(!pLastRole) {
                continue;
            }

            pLastRole->m_nameColor = args[1][0];
        }

        if(args[0] == "set_chat_color") {
            if(!pLastRole) {
                continue;
            }

            pLastRole->m_chatColor = args[1][0];
        }

        if(args[0] == "set_perms") {
            if(!pLastRole) {
                continue;
            }

            for(uint32 i = 1; i < args.size(); ++i) {
                eRolePerm perm;
                if(!GetRolePermFromString(args[i], perm)) {
                    LOGGER_LOG_WARN("Unknown permission %s for %d", args[i].c_str(), pLastRole->GetID());
                    continue;
                }

                pLastRole->AddPerm(perm);
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
    if(!pRole) {
        return false;
    }

    if(pRole->m_state == ROLE_RESOLVE_DONE) {
        return true;
    }

    if(pRole->m_state == ROLE_RESOLVE_PROCESSING) {
        return false;
    }

    pRole->m_state = ROLE_RESOLVE_PROCESSING;
    pRole->m_finalPerms = pRole->m_basePerms;

    for(uint32 parentID : pRole->m_inherits) {
        Role* pParent = GetRole(parentID);
        if(!pParent) {
            LOGGER_LOG_WARN("Non exists role %d in inherits, parent %d", parentID, pRole->GetID());
            continue;
        }

        if(!ResolveRole(pParent)) {
            return false;
        }

        if(pParent->m_finalPerms.size() > pRole->m_finalPerms.size()) {
            pRole->m_finalPerms.resize(pParent->m_finalPerms.size(), 0);
        }

        for(uint32 i = 0; i < pParent->m_finalPerms.size(); ++i) {
            pRole->m_finalPerms[i] |= pParent->m_finalPerms[i];
        }
    }

    pRole->m_state = ROLE_RESOLVE_DONE;
    return true;
}

Role* RoleManager::GetRole(int32 id)
{
    auto it = m_roles.find(id);
    if(it == m_roles.end()) {
        return nullptr;
    }

    return it->second;
}

void InitRolePerms()
{
    /** for now hardcoded */
    sPermStrMap["chat"] = ROLE_PERM_CHAT;
}

bool GetRolePermFromString(const string& permStr, eRolePerm& permOut)
{
    auto it = sPermStrMap.find(permStr);
    if(it == sPermStrMap.end()) {
        return false;
    }

    permOut = it->second;
    return true;
}

RoleManager *GetRoleManager() { return RoleManager::GetInstance(); }
