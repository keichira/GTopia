#pragma once

#include "../Precompiled.h" 
#include "Role.h"

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

    void Kill();

    Role* GetRole(int32 id);
    int32 GetDefaultRoleID() const { return m_defaultRoleID; }

private:
    std::unordered_map<int32, Role*> m_roles;  
    int32 m_defaultRoleID;
};

RoleManager* GetRoleManager();