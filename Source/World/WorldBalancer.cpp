#include "WorldBalancer.h"
#include "../Utils/StringUtils.h"
#include "../Math/Random.h"

WorldBalancer::WorldBalancer()
{

}

WorldBalancer::~WorldBalancer()
{
}

void WorldBalancer::RegisterBalancedWorld(const WorldBalanceConfigSchema& balancer)
{
    if(balancer.worldName.empty() || balancer.maxInstance == 0)
        return;

    m_balancedWorlds.insert_or_assign(balancer.worldName, balancer);
}

WorldBalanceConfigSchema* WorldBalancer::GetBalancerByNameMatch(const string& worldName)
{
    if(worldName.empty())
        return nullptr;

    string upperName = ToUpper(worldName);

    for(auto& info : m_balancedWorlds)
    {
        WorldBalanceConfigSchema& balancer = info.second;
        string name = upperName;

        if(worldName.size() > balancer.worldName.size())
        {
            name = upperName.substr(0, balancer.worldName.size());
        }

        if(name == balancer.worldName)
            return &balancer;
    }

    return nullptr;
}

bool WorldBalancer::IsBalancedWorld(const string& worldName)
{
    WorldBalanceConfigSchema* pBalancer = GetBalancerByNameMatch(worldName);
    if(!pBalancer)
        return false;

    uint32 instanceID = 0;
    bool foundUnder = false;

    if(worldName.size() > pBalancer->worldName.size())
    {
        string suffix = worldName.substr(pBalancer->worldName.size());

        if(!suffix.empty())
        {
            if(suffix[0] == '_')
            {
                suffix = suffix.substr(1);
                foundUnder = true;
            }

            if(!suffix.empty())
            {
                if(ToUInt(suffix, instanceID) != TO_INT_SUCCESS)
                    return false;
            }
        }
    }

    if(instanceID > pBalancer->maxInstance || (pBalancer->keepExactId && !foundUnder))
        return false;
        
    return true;
}

bool WorldBalancer::GetBalancedName(const string& worldName, string& out)
{
    if(!IsBalancedWorld(worldName))
        return false;

    WorldBalanceConfigSchema* pBalancer = GetBalancerByNameMatch(worldName);
    if(!pBalancer)
        return false;
    
    string suffix = worldName.substr(pBalancer->worldName.size());
    if(worldName.size() > pBalancer->worldName.size())
    {
        if(!suffix.empty())
        {
            if(suffix[0] == '_')
            {
                suffix = suffix.substr(1);
            }
        }
    }

    if(!suffix.empty() && pBalancer->keepExactId)
    {
        out = worldName;
        return true;
    }

    out = pBalancer->worldName;
    if(pBalancer->keepExactId)
    {
        out += "_";
    }

    out += ToString(RandomRangeInt(1, pBalancer->maxInstance));
    return true;
}
