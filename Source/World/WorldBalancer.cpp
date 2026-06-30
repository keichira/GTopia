#include "WorldBalancer.h"
#include "../Math/Random.h"
#include "../Utils/StringUtils.h"

WorldBalancer::WorldBalancer() : m_enabled(false) {}

WorldBalancer::~WorldBalancer() {}

bool WorldBalancer::GetBalancedWorldName(const string& worldName, string& out)
{
    return false;
}

void WorldBalancer::RegisterBalancedWorld(WorldBalanceConfigSchema& balancer)
{
    if (balancer.worldName.empty() || balancer.maxInstance == 0)
        return;

    balancer.id = (m_compactWorlds.size() + m_exactWorlds.size());
    balancer.worldName = ToUpper(balancer.worldName);

    if (balancer.keepExactId)
        m_exactWorlds.insert_or_assign(balancer.worldName, balancer);
    else
        m_compactWorlds.insert_or_assign(balancer.worldName, balancer);
}

WorldBalanceConfigSchema* WorldBalancer::GetBalancerByNameMatch(const string& inputName) // wbs
{
    if (inputName.empty())
        return nullptr;

    string worldName = ToUpper(inputName);
    usize underscore = worldName.rfind('_');

    if (underscore != string::npos)
    {
        string prefix = worldName.substr(0, underscore);
        string suffix = worldName.substr(underscore + 1);

        auto it = m_exactWorlds.find(prefix);
        if (it == m_exactWorlds.end())
            return nullptr;

        if (suffix.empty() || suffix.front() == '0')
            return nullptr;

        uint32 id;
        if (ToUInt(suffix, id) != TO_INT_SUCCESS)
            return nullptr;

        if (id == 0 || id > it->second.maxInstance)
            return nullptr;

        return &it->second;
    }

    usize pos = worldName.size();
    while (pos > 0 && IsDigit(worldName[pos - 1]))
        --pos;

    if (pos == worldName.size())
        return nullptr;

    string prefix = worldName.substr(0, pos);
    string suffix = worldName.substr(pos);

    auto it = m_compactWorlds.find(prefix);
    if (it == m_compactWorlds.end())
        return nullptr;

    if (suffix.front() == '0')
        return nullptr;

    uint32 id;
    if (ToUInt(suffix, id) != TO_INT_SUCCESS)
        return nullptr;

    if (id == 0 || id > it->second.maxInstance)
        return nullptr;

    return &it->second;
}

bool WorldBalancer::IsBalancedWorld(const string& worldName)
{
    WorldBalanceConfigSchema* pBalancer = GetBalancerByNameMatch(worldName);
    if (!pBalancer)
        return false;

    uint32 instanceID = 0;
    bool foundUnder = false;

    if (worldName.size() > pBalancer->worldName.size())
    {
        string suffix = worldName.substr(pBalancer->worldName.size());

        if (!suffix.empty())
        {
            if (suffix[0] == '_')
            {
                suffix = suffix.substr(1);
                foundUnder = true;
            }

            if (!suffix.empty())
            {
                if (ToUInt(suffix, instanceID) != TO_INT_SUCCESS)
                    return false;
            }
        }
    }

    if (instanceID > pBalancer->maxInstance || (pBalancer->keepExactId && !foundUnder))
        return false;

    return true;
}

string WorldBalancer::GetBalancedFinalWorldName(const string& worldName)
{
    auto pBalancer = GetBalancerByNameMatch(worldName);
    if (!pBalancer || pBalancer->alwaysCreate)
        return worldName;

    int32 rand = RandomRangeInt(1, pBalancer->maxInstance);

    if (pBalancer->keepExactId)
        return pBalancer->worldName + "_" + ToString(rand);
    else
        return pBalancer->worldName + ToString(rand);

    return worldName;
}

/*bool WorldBalancer::GetBalancedName(const string& worldName, string& out)
{
    if (!IsBalancedWorld(worldName))
        return false;

    WorldBalanceConfigSchema* pBalancer = GetBalancerByNameMatch(worldName);
    if (!pBalancer)
        return false;

    string suffix = worldName.substr(pBalancer->worldName.size());
    if (worldName.size() > pBalancer->worldName.size())
    {
        if (!suffix.empty())
        {
            if (suffix[0] == '_')
            {
                suffix = suffix.substr(1);
            }
        }
    }

    if (!suffix.empty() && pBalancer->keepExactId)
    {
        out = worldName;
        return true;
    }

    out = pBalancer->worldName;
    if (pBalancer->keepExactId)
    {
        out += "_";
    }

    out += ToString(RandomRangeInt(1, pBalancer->maxInstance));
    return true;
}*/
