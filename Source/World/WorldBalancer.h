#pragma once

#include "../Precompiled.h"
#include "../Utils/GameConfig.h"

class WorldBalancer
{
public:
    WorldBalancer();
    ~WorldBalancer();

public:
    virtual bool GetBalancedWorldName(const string& worldName, string& out);

public:
    void SetBalancerEnabled(bool enabled) { m_enabled = enabled; };
    bool IsBalancerEnabled() const { return m_enabled; };

    void RegisterBalancedWorld(WorldBalanceConfigSchema& balancer);

    WorldBalanceConfigSchema* GetBalancerByNameMatch(const string& inputName);
    bool IsBalancedWorld(const string& worldName);
    string GetBalancedFinalWorldName(const string& worldName);
    // bool GetBalancedName(const string& worldName, string& out);

private:
    bool m_enabled;
    std::unordered_map<string, WorldBalanceConfigSchema> m_exactWorlds;
    std::unordered_map<string, WorldBalanceConfigSchema> m_compactWorlds;
};