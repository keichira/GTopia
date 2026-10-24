#pragma once

#include "AchievementManager.h"
#include "Memory/MemoryBuffer.h"
#include "Precompiled.h"
#include "Utils/DialogBuilder.h"

#define PLAYER_PROGRESS_VERSION 2

class GamePlayer;

#define GROWMOJI_LIST(X)                                                                                               \
    X(WL, "(wl)", u8"\u0101")                                                                                          \
    X(YES, "(yes)", u8"\u0102")                                                                                        \
    X(NO, "(no)", u8"\u0103")                                                                                          \
    X(LOVE, "(love)", u8"\u0104")                                                                                      \
    X(OOPS, "(oops)", u8"\u0105")                                                                                      \
    X(SHY, "(shy)", u8"\u0106")                                                                                        \
    X(WINK, "(wink)", u8"\u0107")                                                                                      \
    X(TONGUE, "(tongue)", u8"\u0108")                                                                                  \
    X(AGREE, "(agree)", u8"\u0109")                                                                                    \
    X(SLEEP, "(sleep)", u8"\u010A")                                                                                    \
    X(PUNCH, "(punch)", u8"\u010B")                                                                                    \
    X(MUSIC, "(music)", u8"\u010C")                                                                                    \
    X(BUILD, "(build)", u8"\u010D")                                                                                    \
    X(MEGAPHONE, "(megaphone)", u8"\u010E")                                                                            \
    X(SIGH, "(sigh)", u8"\u010F")                                                                                      \
    X(MAD, "(mad)", u8"\u0110")                                                                                        \
    X(WOW, "(wow)", u8"\u0111")                                                                                        \
    X(DANCE, "(dance)", u8"\u0112")                                                                                    \
    X(SEE_NO_EVIL, "(see-no-evil)", u8"\u0113")                                                                        \
    X(BHEART, "(bheart)", u8"\u0114")                                                                                  \
    X(HEART, "(heart)", u8"\u0115")                                                                                    \
    X(GROW, "(grow)", u8"\u0116")                                                                                      \
    X(GEMS, "(gems)", u8"\u0117")                                                                                      \
    X(KISS, "(kiss)", u8"\u0118")                                                                                      \
    X(GTOKEN, "(gtoken)", u8"\u0119")                                                                                  \
    X(LOL, "(lol)", u8"\u011A")                                                                                        \
    X(SMILE, "(smile)", u8"\u0100")                                                                                    \
    X(COOL, "(cool)", u8"\u011C")                                                                                      \
    X(CRY, "(cry)", u8"\u011D")                                                                                        \
    X(VEND, "(vend)", u8"\u011E")                                                                                      \
    X(BUNNY, "(bunny)", u8"\u011B")                                                                                    \
    X(CACTUS, "(cactus)", u8"\u011F")                                                                                  \
    X(PINE, "(pine)", u8"\u0124")                                                                                      \
    X(PEACE, "(peace)", u8"\u0123")                                                                                    \
    X(TERROR, "(terror)", u8"\u0121")                                                                                  \
    X(TROLL, "(troll)", u8"\u0120")                                                                                    \
    X(EVIL, "(evil)", u8"\u0122")                                                                                      \
    X(FIREWORKS, "(fireworks)", u8"\u0126")                                                                            \
    X(FOOTBALL, "(football)", u8"\u0125")                                                                              \
    X(ALIEN, "(alien)", u8"\u0127")                                                                                    \
    X(PARTY, "(party)", u8"\u0128")                                                                                    \
    X(PIZZA, "(pizza)", u8"\u0129")                                                                                    \
    X(CLAP, "(clap)", u8"\u012A")                                                                                      \
    X(SONG, "(song)", u8"\u012B")                                                                                      \
    X(GHOST, "(ghost)", u8"\u012C")                                                                                    \
    X(NUKE, "(nuke)", u8"\u012D")                                                                                      \
    X(HALO, "(halo)", u8"\u012E")                                                                                      \
    X(TURKEY, "(turkey)", u8"\u012F")                                                                                  \
    X(GIFT, "(gift)", u8"\u0130")                                                                                      \
    X(CAKE, "(cake)", u8"\u0131")                                                                                      \
    X(HEARTARROW, "(heartarrow)", u8"\u0132")                                                                          \
    X(LUCKY, "(lucky)", u8"\u0133")                                                                                    \
    X(SHAMROCK, "(shamrock)", u8"\u0134")                                                                              \
    X(GRIN, "(grin)", u8"\u0135")                                                                                      \
    X(ILL, "(ill)", u8"\u0136")                                                                                        \
    X(EYES, "(eyes)", u8"\u0137")                                                                                      \
    X(WEARY, "(weary)", u8"\u0138")                                                                                    \
    X(MOYAI, "(moyai)", u8"\u013C")                                                                                    \
    X(PLEAD, "(plead)", u8"\u013D")                                                                                    \
    X(FIRE, "(fire)", u8"\u013E")                                                                                      \
    X(FLIRTY, "(flirty)", u8"\u013F")                                                                                  \
    X(LIGHTBULB, "(lightbulb)", u8"\u0140")                                                                            \
    X(MONEYMOUTH, "(moneymouth)", u8"\u0141")                                                                          \
    X(NERD, "(nerd)", u8"\u0142")                                                                                      \
    X(SKULL, "(skull)", u8"\u0143")                                                                                    \
    X(TROPHY, "(trophy)", u8"\u0144")                                                                                  \
    X(WILTED, "(wilted)", u8"\u0145")

enum eGrowmoji : uint16
{
#define X(name, code, glyph) GROWMOJI_##name,
    GROWMOJI_LIST(X)
#undef X
    GROWMOJI_COUNT
};

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
    static constexpr uint32 GROWMOJI_BLOCK_COUNT = (GROWMOJI_COUNT + 31) / 32;

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

    bool HasGrowmoji(eGrowmoji emo) const;
    void GiveGrowmoji(eGrowmoji emo);
    void RemoveGrowmoji(eGrowmoji emo);
    eGrowmoji FindEmoticonByCode(const string& code);
    bool CanUseGrowmojisInText(const string& text);
    string GetEmoticonPacket();

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
    uint32 m_growmojis[GROWMOJI_BLOCK_COUNT];
};