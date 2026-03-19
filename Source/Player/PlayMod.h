#pragma once

#include "../Precompiled.h"
#include "../Math/Color.h"

enum ePlayModType
{
    PLAYMOD_TYPE_NONE,
    PLAYMOD_TYPE_STEALTH,
    PLAYMOD_TYPE_BLUEBERRY,
    PLAYMOD_TYPE_GHOST,
    PLAYMOD_TYPE_DOUBLE_JUMP,
    PLAYMOD_TYPE_THE_ONE_RING,
    PLAYMOD_TYPE_LEGENDARY,
    PLAYMOD_TYPE_SPEEDY,
    PLAYMOD_TYPE_CARRYING_A_TORCH,
    PLAYMOD_TYPE_ENERGIZED_HORN,
    PLAYMOD_TYPE_FOCUSED_EYES
};

class PlayMod {
public:
    friend class PlayModManager;

public:
    PlayMod();
    ~PlayMod();

public:
    ePlayModType GetType() const { return m_modType; }
    uint8 GetPunchType() const { return m_punchType; }
    uint16 GetDisplayItem() const { return m_displayItem; }
    uint32 GetCharFlags() const { return m_charFlags; }
    uint32 GetTime() const { return m_durationTime; }

    uint8 GetBuildRange() const { return m_buildRange; }

    float GetPunchDamage() const { return m_punchDamage; }
    float GetSpeed() const { return m_speed; }
    float GetPunchPower() const { return m_punchPower; }

    const string& GetName() const { return m_name; }
    const string& GetAddMessage() const { return m_addMessage; }
    const string& GetRemoveMessage() const { return m_removeMessage; }
    Color& GetSkinColor() { return m_skinColor; }

private:
    ePlayModType m_modType;
    uint8 m_punchType;

    uint16 m_displayItem;
    string m_name;
    string m_addMessage;
    string m_removeMessage;

    float m_punchDamage;
    float m_punchPower;
    float m_speed;

    uint8 m_buildRange;

    uint32 m_charFlags;
    Color m_skinColor;

    uint32 m_durationTime;
};