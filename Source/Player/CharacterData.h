#pragma once

#include "../Precompiled.h"
#include "../Math/Color.h"
#include "PlayMod.h"
#include "../Utils/Timer.h"

#define CHARACTER_DEFAULT_FIRE_DAMAGE 1.0f // umm
#define CHARACTER_DEFAULT_SPEED 1000.0f
#define CHARACTER_DEFAULT_WATER_SPEED 125.0f
#define CHARACTER_DEFAULT_GRAVITY 1000.0f
#define CHARACTER_DEFAULT_ACCEL 250.0f
#define CHARACTER_DEFAULT_PUNCH_POWER 200.0f
#define CHARACTER_DEFAULT_PUNCH_DAMAGE 6.0f

enum ePlayerFlag
{
    PLAYER_FLAG_FACING_LEFT = 1 << 0,
    PLAYER_FLAG_INVISIBLE = 1 << 1
};

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
    CHAR_STATE_INVISIBLE = 1 << 2
};

class GamePlayer;

class CharacterData {
public:
    CharacterData();
    ~CharacterData();

public:
    bool HasCharFlag(uint32 flag) { return m_charFlags & flag; };
    void SetCharFlag(uint32 flag) { m_charFlags |= flag; }
    void RemoveCharFlag(uint32 flag) { m_charFlags &= ~flag; }

    bool HasPlayerFlag(uint32 flag) { return m_playerFlags & flag; };
    void SetPlayerFlag(uint32 flag) { m_playerFlags |= flag; }
    void RemovePlayerFlag(uint32 flag) { m_playerFlags &= ~flag; }

    uint8 GetPunchType() const { return m_punchType; }
    uint8 GetPunchRange() const { return m_punchRange; }
    uint8 GetBuildRange() const { return m_buildRange; }
    uint32 GetCharFlags() const { return m_charFlags; }

    float GetPunchPower() const { return m_punchPower; }
    float GetPunchDamage() const { return m_punchDamage; }
    float GetGravity() const { return m_gravity; }
    float GetSpeed() const { return m_speed; }
    float GetWaterSpeed() const { return m_waterSpeed; }
    float GetAccel() const { return m_accel; }
    float GetFireDamage() const { return m_fireDamage; }

    uint32 GetSkinColor(bool tint = false);

    bool HasPlayMod(ePlayModType modType);
    PlayerPlayModInfo* GetPlayMod(ePlayModType modType);
    PlayMod* AddPlayMod(ePlayModType modType);
    PlayMod* RemovePlayMod(ePlayModType modType);

    void SetSkinColor(uint32 skinColor) { m_skinColor = skinColor; SetNeedSkinUpdate(true); }

    bool NeededSkinUpdate() const { return m_needSkinUpdate; }
    void SetNeedSkinUpdate(bool val) { m_needSkinUpdate = val; }

    bool NeededCharStateUpdate() const { return m_needCharStateUpdate; }
    void SetNeedCharStateUpdate(bool val) { m_needCharStateUpdate = val; }

    std::vector<PlayerPlayModInfo>& GetReqUpdatePlayMods() { return m_reqUpdatePlayMods; }

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

    uint32 m_charFlags;
    Color m_skinColor;

    uint32 m_playerFlags;

    bool m_needSkinUpdate;
    bool m_needCharStateUpdate;

    std::vector<PlayerPlayModInfo> m_reqUpdatePlayMods;
    std::vector<PlayerPlayModInfo> m_staticPlayMods;
    std::vector<ePlayModType> m_activePlayMods;
};