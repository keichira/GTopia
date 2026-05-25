#pragma once

#include "Precompiled.h"

enum ePlayerProgress
{
    PLAYER_PROGRESS_XP,
    PLAYER_PROGRESS_PLACE_COUNT,
    PLAYER_PROGRESS_PUNCH_COUNT,
    PLAYER_PROGRESS_BREAK_COUNT,
    PLAYER_PROGRESS_HARVEST_COUNT,
    PLAYER_PROGRESS_COLLECT_PROVIDER,

    PLAYER_PROGRESS_COUNT,
    PLAYER_PROGRESS_NONE
};

enum eAchievement
{
    ACHIEVEMENT_BUILDER_1,
    ACHIEVEMENT_BUILDER_2,
    ACHIEVEMENT_BUILDER_3,
    ACHIEVEMENT_FARMER_1,
    ACHIEVEMENT_FARMER_2,
    ACHIEVEMENT_FARMER_3,
    ACHIEVEMENT_DEMOLITION_1,
    ACHIEVEMENT_DEMOLITION_2,
    ACHIEVEMENT_DEMOLITION_3,
    ACHIEVEMENT_HOARDER_1,
    ACHIEVEMENT_HOARDER_2,
    ACHIEVEMENT_HOARDER_3,
    ACHIEVEMENT_SPENDER_1,
    ACHIEVEMENT_SPENDER_2,
    ACHIEVEMENT_SPENDER_3,
    ACHIEVEMENT_TRASH_1,
    ACHIEVEMENT_TRASH_2,
    ACHIEVEMENT_TRASH_3,
    ACHIEVEMENT_CONSUMABLE_1,
    ACHIEVEMENT_CONSUMABLE_2,
    ACHIEVEMENT_CONSUMABLE_3,
    ACHIEVEMENT_BACKPACK_1,
    ACHIEVEMENT_BACKPACK_2,
    ACHIEVEMENT_BACKPACK_3,
    ACHIEVEMENT_LOCK,
    ACHIEVEMENT_ADDEDTOLOCK,
    ACHIEVEMENT_WORLDLOCK,
    ACHIEVEMENT_LEVEL_1,
    ACHIEVEMENT_LEVEL_2,
    ACHIEVEMENT_LEVEL_3,

    ACHIEVEMENT_COUNT
};

enum eAchievementType
{
    ACHIEVE_TYPE_STAT,
    ACHIEVE_TYPE_EVENT
};

struct AchievementInfo
{
    string name;
    string description;

    eAchievement achievement;
    eAchievementType achievementType;
    ePlayerProgress progressType;

    uint32 requiredValue;
};

eAchievementType StrToAchieveType(const string& type);
ePlayerProgress StrToProgressType(const string& type);

class AchievementManager {
public:
    AchievementManager();
    ~AchievementManager();

public:
    static AchievementManager* GetInstance()
    {
        static AchievementManager instance;
        return &instance;
    }

public:
    AchievementInfo* GetAchievement(eAchievement achievement);
    const std::vector<AchievementInfo*>& GetAchievesByProgress(ePlayerProgress progress);

    bool Load(const string& filePath);

private:
    std::vector<AchievementInfo> m_achieves;
    std::vector<AchievementInfo*> m_achievesByProgress[PLAYER_PROGRESS_COUNT];
};

AchievementManager* GetAchievementManager();