#include "AchievementManager.h"
#include "PlayerProgress.h"
#include "Utils/ConfigDB.h"

AchievementManager::AchievementManager()
{
}

AchievementManager::~AchievementManager()
{
}

eAchievementType StrToAchieveType(const string& type)
{
    if(type == "TYPE_STAT")
        return ACHIEVE_TYPE_STAT;

    return ACHIEVE_TYPE_EVENT;
}

ePlayerProgress StrToProgressType(const string& type)
{
    static const std::unordered_map<string, ePlayerProgress> progressTypeMap 
    {
        { "XP", PLAYER_PROGRESS_XP },
        { "PLACE_COUNT", PLAYER_PROGRESS_PLACE_COUNT },
        { "PUNCH_COUNT", PLAYER_PROGRESS_PUNCH_COUNT },
        { "NONE", PLAYER_PROGRESS_NONE },
        { "BREAK_COUNT", PLAYER_PROGRESS_BREAK_COUNT },
        { "HARVEST_COUNT", PLAYER_PROGRESS_HARVEST_COUNT }
    };

    auto it = progressTypeMap.find(type);
    if(it != progressTypeMap.end()) {
        return it->second;
    }

    return PLAYER_PROGRESS_NONE;
}

AchievementInfo* AchievementManager::GetAchievement(eAchievement achievement)
{
    if(achievement >= m_achieves.size() || achievement >= ACHIEVEMENT_COUNT)
        return nullptr;

    return &m_achieves[achievement];
}

const std::vector<AchievementInfo*>& AchievementManager::GetAchievesByProgress(ePlayerProgress progress)
{
    return m_achievesByProgress[progress];
}

bool AchievementManager::Load(const string& filePath)
{
    ConfigDB cfg;
    if(!cfg.Load(filePath))
        return false;

    for(auto& line : cfg.Lines())
    {
        if(!line.Require(5))
            return false;

        AchievementInfo achi =
        {
            line.GetString(1),
            line.GetString(2),
            (eAchievement)line.GetUInt(0),
            StrToAchieveType(line.GetString(3)),
            StrToProgressType(line.GetString(4)),
            line.GetUInt(5)
        };

        m_achieves.push_back(std::move(achi));
    }

    for(uint32 i = 0; i < m_achieves.size(); ++i)
    {
        AchievementInfo* pAchieve = &m_achieves[i];
        if(!pAchieve || pAchieve->progressType == PLAYER_PROGRESS_NONE)
            continue;

        m_achievesByProgress[pAchieve->progressType].push_back(pAchieve);
    }

    return true;
}

AchievementManager* GetAchievementManager() { return AchievementManager::GetInstance(); }