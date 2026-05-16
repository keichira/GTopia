#pragma once

#include "../Precompiled.h"
#include "../Math/Color.h"
#include "PlayMod.h"
#include "../Math/Vector2.h"
#include "../Utils/Timer.h"
#include "../Utils/DialogBuilder.h"

#define CHARACTER_DEFAULT_FIRE_DAMAGE 1.0f // umm
#define CHARACTER_DEFAULT_SPEED 1000.0f
#define CHARACTER_DEFAULT_WATER_SPEED 125.0f
#define CHARACTER_DEFAULT_GRAVITY 1000.0f
#define CHARACTER_DEFAULT_ACCEL 250.0f
#define CHARACTER_DEFAULT_PUNCH_POWER 200.0f
#define CHARACTER_DEFAULT_PUNCH_DAMAGE 6.0f


struct PlayerPlayModInfo
{
    ePlayModType modType = PLAYMOD_TYPE_NONE;
    Timer timer;
    uint32 durationMS = 0;
};

enum eCharacterStateFlag
{
    CHAR_STATE_NO_CLIP = 1 << 0,
    CHAR_STATE_DOUBLE_JUMP = 1 << 1,
    CHAR_STATE_RENDER_EYES_ONLY = 1 << 2,
    CHAR_STATE_NO_ARMS = 1 << 3,
    CHAR_STATE_NO_FACE = 1 << 4,
    CHAR_STATE_NO_HEAD_AND_TORSO = 1 << 5,
    CHAR_STATE_DEVIL_HORNS = 1 << 6,
    CHAR_STATE_HALO = 1 << 7,
    CHAR_STATE_HIGH_JUMP = 1 << 8,
    CHAR_STATE_FIREPROOF = 1 << 9,
    CHAR_STATE_SPIKEPROOF = 1 << 10,
    CHAR_STATE_FROZEN = 1 << 11,
    CHAR_STATE_WORLD_LOCKED = 1 << 12,
    CHAR_STATE_DUCT_TAPED = 1 << 13,
    CHAR_STATE_STINKY = 1 << 14,
    CHAR_STATE_SPARKLY = 1 << 15,
    CHAR_STATE_ZOMBIFIED = 1 << 16,
    CHAR_STATE_ON_FIRE = 1 << 17,
    CHAR_STATE_SHADOW_HAUNTED = 1 << 18,
    CHAR_STATE_IRRADIATED = 1 << 19,
    CHAR_STATE_SPOTLIGHT = 1 << 20,
    CHAR_STATE_PINEAPPLE1 = 1 << 21,
    CHAR_STATE_PINEAPPLE2 = 1 << 22,
    CHAR_STATE_PINEAPPLE_AURA = 1 << 23,
    CHAR_STATE_SSUPPORTER = 1 << 24,
    CHAR_STATE_SUPER_PINEAPPLE_AURA = 1 << 25,
    CHAR_STATE_BALLOON_WAR_SHIELD = 1 << 26,
    CHAR_STATE_SOAKED = 1 << 27
};

enum eCharacterFlags
{
    CHARACTER_FLAG_FACING_LEFT = 1 << 0,
    CHARACTER_FLAG_INVISIBLE = 1 << 1
};

class GamePlayer;

class CharacterData {
public:
    CharacterData();
    ~CharacterData();

public:
    bool HasCharState(uint32 flag) { return m_charState & flag; };
    void SetCharState(uint32 flag) { m_charState |= flag; }
    void RemoveCharState(uint32 flag) { m_charState &= ~flag; }

    bool HasCharFlag(uint32 flag) { return m_charFlags & flag; };
    void SetCharFlag(uint32 flag) { m_charFlags |= flag; }
    void RemoveCharFlag(uint32 flag) { m_charFlags &= ~flag; }

    uint8 GetPunchType() const { return m_punchType; }
    uint8 GetPunchRange() const { return m_punchRange; }
    uint8 GetBuildRange() const { return m_buildRange; }
    uint32 GetCharFlags() const { return m_charState; }

    float GetPunchPower() const { return m_punchPower; }
    float GetPunchDamage() const { return m_punchDamage; }
    float GetGravity() const { return m_gravity; }
    float GetSpeed() const { return m_speed; }
    float GetWaterSpeed() const { return m_waterSpeed; }
    float GetAccel() const { return m_accel; }
    float GetFireDamage() const { return m_fireDamage; }

    uint32 GetSkinColor(bool tint = false);
    void GetActiveModsForDialog(DialogBuilder& db);

    bool HasPlayMod(ePlayModType modType);
    PlayerPlayModInfo* GetPlayMod(ePlayModType modType);
    PlayMod* AddPlayMod(ePlayModType modType);
    PlayMod* RemovePlayMod(ePlayModType modType);

    void SetSkinColor(uint32 skinColor) { m_skinColor.SetUINTSwap(skinColor); }

    bool NeededSkinUpdate() const { return m_needSkinUpdate; }
    void SetNeedSkinUpdate(bool val) { m_needSkinUpdate = val; }

    bool NeededCharStateUpdate() const { return m_needCharStateUpdate; }
    void SetNeedCharStateUpdate(bool val) { m_needCharStateUpdate = val; }

    std::vector<PlayerPlayModInfo>& GetReqUpdatePlayMods() { return m_reqUpdatePlayMods; }
    uint32 GetActiveModCount() { return m_activePlayMods.size(); };

private:
    void RemovePlayMod(PlayerPlayModInfo* pPlayerMod);
    void SetNeededUpdates(PlayMod* pPlayMod);
    void GetNextPunchType();

private:
    uint8 m_punchType;
    uint8 m_punchRange;
    uint8 m_buildRange;

    float m_punchPower;
    float m_punchDamage;
    float m_gravity;
    float m_speed;
    float m_waterSpeed;
    float m_accel;
    float m_fireDamage;

    uint32 m_charState;
    Color m_skinColor;

    uint32 m_charFlags;

    bool m_needSkinUpdate;
    bool m_needCharStateUpdate;

    Vector2Int m_avatarSize;

    std::vector<PlayerPlayModInfo> m_reqUpdatePlayMods;
    std::vector<PlayerPlayModInfo> m_staticPlayMods;
    std::vector<ePlayModType> m_activePlayMods;
};