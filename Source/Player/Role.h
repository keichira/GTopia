#pragma once

#include "../Precompiled.h"

enum eRolePerm;

enum eRoleResolve
{
    ROLE_RESOLVE_NONE,
    ROLE_RESOLVE_PROCESSING,
    ROLE_RESOLVE_DONE
};

class Role {
public:
    friend class RoleManager;

public:
    Role();

public:
    const string& GetName() const { return m_name; }

    int32 GetID() const { return m_id; }
    const string& GetPrefix() const { return m_prefix; }
    const string& GetSuffix() const { return m_suffix; }
    char GetNameColor() const { return m_nameColor; }
    char GetChatColor() const { return m_chatColor; }

    bool HasPerm(eRolePerm perm);

private:
    void AddPerm(eRolePerm perm);

private:
    string m_name;
    int32 m_id;
    string m_prefix;
    string m_suffix;
    char m_nameColor;
    char m_chatColor;
    std::vector<int32> m_inherits;

    eRoleResolve m_state;
    
    std::vector<uint32> m_basePerms;
    std::vector<uint32> m_finalPerms;
};