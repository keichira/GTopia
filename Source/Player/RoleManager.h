#pragma once

#include "../Precompiled.h" 
#include "Role.h"

enum eRolePerm
{
    ROLE_PERM_CHAT,

    ROLE_PERM_SIZE
};

static std::unordered_map<string, eRolePerm> sPermStrMap;
void InitRolePerms();
bool GetRolePermFromString(const string& permStr, eRolePerm& permOut);

class RoleManager {
public:
    RoleManager();
    ~RoleManager();

public:
    static RoleManager* GetInstance() {
        static RoleManager instance;
        return &instance;
    }

public:
    bool Load(const string& filePath);
    bool ResolveRole(Role* pRole);

    Role* GetRole(int32 id);

private:
    std::unordered_map<int32, Role*> m_roles;  
};

RoleManager* GetRoleManager();