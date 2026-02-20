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
    static bool IsValidPerm(uint32 perm);

public:
    bool Load(const string& filePath);
    void CheckInherits();

    Role* GetRole(int32 id);

private:
    std::unordered_map<int32, Role*> m_roles;  
};

RoleManager* GetRoleManager();