#pragma once

#include "Precompiled.h"

enum ePlayerProgress
{
    PLAYER_PROGRESS_XP,
    PLAYER_PROGRESS_PLACE_COUNT,
    PLAYER_PROGRESS_PUNCH_COUNT,
    PLAYER_PROGRESS_BREAK_COUNT,
    PLAYER_PROGRESS_HARVEST_COUNT,

    PLAYER_PROGRESS_COUNT,
    PLAYER_PROGRESS_NONE
};

enum eAchievement
{
    ACHIEVEMENT_BUILDER_1,
    ACHIEVEMENT_BUILDER_2,
    ACHIEVEMENT_BUILDER_3,
    ACHIEVE_FARMER1,
    ACHIEVE_FARMER2,
    ACHIEVE_FARMER3,
    ACHIEVE_DEMOLITION1,
    ACHIEVE_DEMOLITION2,
    ACHIEVE_DEMOLITION3,
    ACHIEVE_HOARDER1,
    ACHIEVE_HOARDER2,
    ACHIEVE_HOARDER3,
    ACHIEVE_SPENDER1,
    ACHIEVE_SPENDER2,
    ACHIEVE_SPENDER3,
    ACHIEVE_TRASH1,
    ACHIEVE_TRASH2,
    ACHIEVE_TRASH3,
    ACHIEVE_CONSUMABLE1,
    ACHIEVE_CONSUMABLE2,
    ACHIEVE_CONSUMABLE3,
    ACHIEVE_BACKPACK1,
    ACHIEVE_BACKPACK2,
    ACHIEVE_BACKPACK3,
    ACHIEVE_LOCK,
    ACHIEVE_ADDEDTOLOCK,
    ACHIEVE_WORLDLOCK,
    ACHIEVE_LEVEL1,
    ACHIEVE_LEVEL2,
    ACHIEVE_LEVEL3,

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