#include "PlayerProgress.h"
#include "AchievementManager.h"
#include "Math/Math.h"
#include "GamePlayer.h"
#include "../World/WorldManager.h"

PlayerProgress::PlayerProgress(GamePlayer* pPlayer)
: m_pPlayer(pPlayer), m_version(PLAYER_PROGRESS_VERSION)
{
    memset(m_progressData, 0, sizeof(m_progressData));
    memset(m_achieves, 0, sizeof(m_achieves));

    m_progressData[PLAYER_PROGRESS_XP] = 150;
}

void PlayerProgress::Serialize(MemoryBuffer& memBuffer, bool write)
{
    memBuffer.ReadWrite(m_version, write);

    uint16 progressCount = PLAYER_PROGRESS_COUNT;
    memBuffer.ReadWrite(progressCount, write);
    memBuffer.ReadWriteRaw(m_progressData, sizeof(uint32) * progressCount, write);

    uint16 achieveBlockCount = ACHIEVEMENT_BLOCK_COUNT;
    memBuffer.ReadWrite(achieveBlockCount, write);
    memBuffer.ReadWriteRaw(m_achieves, sizeof(uint32) * achieveBlockCount, write);
}

uint32 PlayerProgress::GetMemEstimate()
{
    return sizeof(m_version) + sizeof(uint16) + sizeof(m_progressData) + sizeof(uint16) + sizeof(m_achieves);
}

uint32 PlayerProgress::GetProgress(ePlayerProgress progress) const
{
    if(progress >= PLAYER_PROGRESS_COUNT)
        return 0;

    return m_progressData[progress];
}

void PlayerProgress::AddProgress(ePlayerProgress progress, uint32 count)
{
    if(progress >= PLAYER_PROGRESS_COUNT)
        return;

    m_progressData[progress] += count;
    CheckAchieveAndUnlockIfPossibleByProgress(progress);
}

void PlayerProgress::SetProgress(ePlayerProgress progress, uint32 value)
{
    if(progress >= PLAYER_PROGRESS_COUNT)
        return;

    m_progressData[progress] = value;
    CheckAchieveAndUnlockIfPossibleByProgress(progress);
}

bool PlayerProgress::HasAchievement(eAchievement achievement)
{
    if(achievement >= ACHIEVEMENT_COUNT)
        return false;

    int32 index = (int32)achievement;
    return m_achieves[index / 32] & (1 << (index % 32));
}

float PlayerProgress::GetAchievementProgress(eAchievement achievement)
{
    if(achievement >= ACHIEVEMENT_COUNT)
        return 0.0f;

    if(HasAchievement(achievement))
        return 100.0f;

    AchievementInfo* pAchieve = GetAchievementManager()->GetAchievement(achievement);
    if(!pAchieve)
        return 0.0f;

    if(pAchieve->achievementType == ACHIEVE_TYPE_EVENT)
        return 0.0f;

    if(pAchieve->achievementType == ACHIEVE_TYPE_STAT)
    {
        if(pAchieve->requiredValue == 0)
            return 0.0f;

        uint32 progress = Min(pAchieve->requiredValue, GetProgress(pAchieve->progressType));
        return (progress / pAchieve->requiredValue) * 100.0f;
    }

    return 0.0f;
}

void PlayerProgress::UnlockAchievement(eAchievement achievement)
{
    AchievementInfo* pAchievement = GetAchievementManager()->GetAchievement(achievement);
    if(!pAchievement)
        return;

    if(HasAchievement(achievement))
        return;

    UnlockAchievementRaw(achievement);
    
    if(m_pPlayer)
    {
        m_pPlayer->GiveXP(100);

        World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_pPlayer->GetCurrentWorld());
        
        if(!pWorld)
        {
            m_pPlayer->SendOnConsoleMessage("You got the achievement \"" + pAchievement->name + "\"!");
            return;
        }

        pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_ACHIEVE, m_pPlayer->GetWorldPos());
        pWorld->SendTalkBubbleAndConsoleToAll(
            m_pPlayer->GetDisplayName(false) + " `5earned the achievement \"" + pAchievement->name + "\"!``", false, m_pPlayer
        );
    }
}

void PlayerProgress::CheckAchieveAndUnlockIfPossibleByProgress(ePlayerProgress progress)
{
    if(progress >= PLAYER_PROGRESS_COUNT)
        return;

    auto achieves = GetAchievementManager()->GetAchievesByProgress(progress);
    
    for(auto& pAchieve : achieves)
    {
        if(!pAchieve || HasAchievement(pAchieve->achievement))
            continue;

        if(GetAchievementProgress(pAchieve->achievement) >= 100.0f)
        {
            UnlockAchievement(pAchieve->achievement);
        }
    }
}

uint16 PlayerProgress::BuildAchievementsDialog(DialogBuilder& db, bool onlyAchieved)
{
    uint32 count = 0;
    AchievementManager* pAchiMgr = GetAchievementManager();

    for(uint16 i = 0; i < ACHIEVEMENT_COUNT; ++i)
    {
        if(onlyAchieved && !HasAchievement((eAchievement)i))
            continue;

        AchievementInfo* pConfig = pAchiMgr->GetAchievement((eAchievement)i);
        if(!pConfig)
            continue;

        if(GetAchievementProgress((eAchievement)i) >= 100.0f)
        {
            db.AddAchieveButton(pConfig->name, "Earned for " + pConfig->description, i);
        }

        count++;
    }

    return count;
}

void PlayerProgress::UnlockAchievementRaw(eAchievement achievement)
{
    if(achievement >= ACHIEVEMENT_COUNT)
        return;

    int32 index = (int32)achievement;
    m_achieves[index / 32] |= (1 << (index % 32));
}
