#pragma once

#include "../Precompiled.h"
#include "../Utils/GameConfig.h"

class WorldBalancer {
public:
    WorldBalancer();
    ~WorldBalancer();

public:
    void SetBalancerEnabled(bool enabled) { m_enabled = enabled; };
    bool IsBalancerEnabled() const { return m_enabled; };

    void RegisterBalancedWorld(const WorldBalanceConfigSchema& balancer);

    WorldBalanceConfigSchema* GetBalancerByNameMatch(const string& worldName);
    bool IsBalancedWorld(const string& worldName);
    bool GetBalancedName(const string& worldName, string& out);

private:
    bool m_enabled;
    std::unordered_map<string, WorldBalanceConfigSchema> m_balancedWorlds;
};