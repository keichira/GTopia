#pragma once

#include "../Precompiled.h"

enum eRolePerms {
    ROLE_PERM_ALL = 3641042151u,
    ROLE_PERM_ADMIN_ALL = 3168208460u,
    ROLE_PERM_PLAYER_ALL = 2898756319u,
    ROLE_PERM_WORLD_ALL = 1383672981u,
};

class Role {
public:
    Role();

public:
    const string& GetName() const { return m_name; }
    void SetName(const string& name) { m_name = name; }

    int32 GetID() const { return m_id; }
    void SetID(int32 id) { m_id = m_id; };

    int32 GetPriority() const { return m_priority; }
    void SetPriority(int32 priority) { m_priority = priority; }

    const string& GetPrefix() const { return m_prefix; }
    void SetPrefix(const string& prefix) { m_prefix = prefix; }

    const string& GetSuffix() const { return m_suffix; }
    void SetSuffix(const string& suffix) { m_suffix = suffix; }

    char GetNameColor() const { return m_nameColor; }
    void SetNameColor(char color) { m_nameColor = color; }

    char GetChatColor() const { return m_chatColor; }
    void SetChatColor(char color) { m_chatColor = color; }
    
    void SetPerms(std::vector<string>& perms);
    bool HasPerm(eRolePerms rolePerm);

    void AddInherit(int32 roleID) { m_inherits.push_back(roleID); }
    const std::vector<int32> GetInherits() const { return m_inherits; }

private:
    string m_name;
    int32 m_id;
    int32 m_priority;
    string m_prefix;
    string m_suffix;
    char m_nameColor;
    char m_chatColor;
    std::vector<int32> m_inherits;
    
    std::vector<uint32> m_perms;
};