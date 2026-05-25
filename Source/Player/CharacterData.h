#pragma once

#include "../Precompiled.h"
#include "../Math/Color.h"
#include "../Math/Vector2.h"

#define CHARACTER_DEFAULT_FIRE_DAMAGE 1.0f // umm
#define CHARACTER_DEFAULT_SPEED 1000.0f
#define CHARACTER_DEFAULT_WATER_SPEED 125.0f
#define CHARACTER_DEFAULT_GRAVITY 1000.0f
#define CHARACTER_DEFAULT_ACCEL 250.0f
#define CHARACTER_DEFAULT_PUNCH_POWER 200.0f
#define CHARACTER_DEFAULT_PUNCH_DAMAGE 6.0f

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
    CharacterData() { ResetToBaseStats(); }

public:
    void ResetToBaseStats()
    {
        charState = 0;
        speed = CHARACTER_DEFAULT_SPEED;
        waterSpeed = CHARACTER_DEFAULT_WATER_SPEED;
        gravity = CHARACTER_DEFAULT_GRAVITY;
        accel = CHARACTER_DEFAULT_ACCEL;
        punchPower = CHARACTER_DEFAULT_PUNCH_POWER;
        punchDamage = CHARACTER_DEFAULT_PUNCH_DAMAGE;
        buildRange = 128;
        punchRange = 128;
        punchType = 0;
        skinColor = Color(180, 138, 120, 255);
        avatarSize = Vector2Int(20, 30);

        needSkinUpdate = false;
        needCharStateUpdate = false;
    }

    bool HasCharState(uint32 state) { return charState & state; };
    void SetCharState(uint32 state) { charState |= state; }
    void RemoveCharState(uint32 state) { charState &= ~state; }

    bool HasCharFlag(uint32 flag) { return charFlags & flag; };
    void SetCharFlag(uint32 flag) { charFlags |= flag; }
    void RemoveCharFlag(uint32 flag) { charFlags &= ~flag; }

public:
    uint8 punchType;
    uint8 punchRange;
    uint8 buildRange;
    float punchPower;
    float punchDamage;
    float gravity;
    float speed;
    float waterSpeed;
    float accel;
    float fireDamage;
    Color skinColor;
    uint32 charState;
    uint32 charFlags;

    bool needSkinUpdate;
    bool needCharStateUpdate;

    Vector2Int avatarSize;
};