#pragma once

#include "Precompiled.h"
#include "AchievementManager.h"
#include "Memory/MemoryBuffer.h"
#include "Utils/DialogBuilder.h"

#define PLAYER_PROGRESS_VERSION 1

class GamePlayer;

class PlayerProgress {
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

    uint16 BuildAchievementsDialog(DialogBuilder& db, bool onlyAchieved);

private:
    void UnlockAchievementRaw(eAchievement achievement);

private:
    uint16 m_version;

    GamePlayer* m_pPlayer;
    uint32 m_progressData[PLAYER_PROGRESS_COUNT];
    uint32 m_achieves[ACHIEVEMENT_BLOCK_COUNT];
};