#include "RoleManager.h"
#include "../IO/File.h"
#include "../Utils/StringUtils.h"
#include "../IO/Log.h"

RoleManager::RoleManager()
{
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
            pRole->SetName(args[1]);
            pRole->SetID(ToInt(args[2]));
            pRole->SetPriority(ToInt(args[3]));
            pRole->SetPrefix(args[4]);
            pRole->SetSuffix(args[5]);

            m_roles.insert_or_assign(pRole->GetID(), pRole);
        }

        if(args[0] == "set_inherits") {
            if(!pLastRole) {
                continue;
            }

            for(uint8 i = 1; i < args.size(); ++i) {
                pLastRole->AddInherit(ToInt(args[i]));
            }
        }

        if(args[0] == "set_name_color") {
            if(!pLastRole) {
                continue;
            }

            pLastRole->SetNameColor(args[1][0]);
        }

        if(args[0] == "set_chat_color") {
            if(!pLastRole) {
                continue;
            }

            pLastRole->SetChatColor(args[1][0]);
        }

        if(args[0] == "set_perms") {
            if(!pLastRole) {
                continue;
            }

            pLastRole->SetPerms(args);
        }
    }

    CheckInherits();
    return true;
}

void RoleManager::CheckInherits()
{
    for(auto& [_, pRole] : m_roles) {
        auto inherits = pRole->GetInherits();

        for(auto& inherit : inherits) {
            if(!GetRole(inherit)) {
                LOGGER_LOG_WARN("Non exist role %d in inherits, parent: %d", inherit, pRole->GetID());
            }
        }
    }
}

Role* RoleManager::GetRole(int32 id)
{
    auto it = m_roles.find(id);
    if(it == m_roles.end()) {
        return nullptr;
    }

    return it->second;
}

bool RoleManager::IsValidPerm(uint32 perm)
{
    return false;
}

RoleManager* GetRoleManager() { return RoleManager::GetInstance(); }
