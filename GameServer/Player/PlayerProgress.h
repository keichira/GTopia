#pragma once

#include "AchievementManager.h"
#include "Memory/MemoryBuffer.h"
#include "Precompiled.h"
#include "Utils/DialogBuilder.h"

#define PLAYER_PROGRESS_VERSION 1

class GamePlayer;

enum ePlayerTitle // temp gonna move to extraData?
{
    PLAYER_TITLE_LEGEND = 1 << 0,
    PLAYER_TITLE_DOCTOR = 1 << 1,
    PLAYER_TITLE_MAX_LVL = 1 << 2,
    PLAYER_TITLE_MASTER = 1 << 3,
    PLAYER_TITLE_G4G = 1 << 4,
    PLAYER_TITLE_THANKSGIVING = 1 << 5,
    PLAYER_TITLE_ANNIVERSARY = 1 << 6,
    PLAYER_TITLE_PARTY = 1 << 7
};

class PlayerProgress
{
public:
    static constexpr uint32 ACHIEVEMENT_BLOCK_COUNT = (ACHIEVEMENT_COUNT + 31) / 32;

public:
    PlayerProgress(GamePlayer* pPlayer);

public:
    void Serialize(MemoryBuffer& memBuffer, bool write);
    uint32 GetMemEstimate();

    uint32 GetProgress(ePlayerProgress progress) const;
    void AddProgress(ePlayerProgress progress, uint32 count);
    void SetProgress(ePlayerProgress progress, uint32 value);

    bool HasAchievement(eAchievement achievement);
    float GetAchievementProgress(eAchievement achievement);
    void UnlockAchievement(eAchievement achievement);
    void CheckAchieveAndUnlockIfPossibleByProgress(ePlayerProgress progress);
    uint32 GetCountOfCompletedAchieves();

    bool HasTitle(ePlayerTitle title);
    void ModifyOwnedTitle(ePlayerTitle title, bool add);
    bool IsTitleActive(ePlayerTitle title);
    void ModifyTitleActivation(ePlayerTitle title, bool activate);

    uint16 BuildAchievementsDialog(DialogBuilder& db, bool onlyAchieved);
    string GetBattlePetName(int32 slot);
    bool IsBattleLeashFull();

private:
    void UnlockAchievementRaw(eAchievement achievement);

private:
    uint16 m_version;

    GamePlayer* m_pPlayer;
    uint32 m_progressData[PLAYER_PROGRESS_COUNT];
    uint32 m_achieves[ACHIEVEMENT_BLOCK_COUNT];
};