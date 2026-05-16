#include "CharacterData.h"
#include "PlayModManager.h"
#include "../IO/Log.h"
#include "../Item/ItemUtils.h"

CharacterData::CharacterData()
: m_charState(0), m_charFlags(0),
m_accel(CHARACTER_DEFAULT_ACCEL), m_gravity(CHARACTER_DEFAULT_GRAVITY),
m_punchPower(CHARACTER_DEFAULT_PUNCH_POWER), 
m_speed(CHARACTER_DEFAULT_SPEED), m_waterSpeed(CHARACTER_DEFAULT_WATER_SPEED),
m_buildRange(128), m_punchRange(128), m_punchType(0), m_punchDamage(CHARACTER_DEFAULT_PUNCH_DAMAGE),
m_needSkinUpdate(false), m_needCharStateUpdate(false),
m_skinColor(180, 138, 120, 255)
{
}

CharacterData::~CharacterData()
{
}

uint32 CharacterData::GetSkinColor(bool tint)
{
    if(!tint) {
        return m_skinColor.GetAsUINTSwap();
    }

    PlayModManager* pModMgr = GetPlayModManager();
    Color finalColor = m_skinColor;

    for(auto& playMod : m_activePlayMods) {
        PlayMod* pPlayMod = pModMgr->GetPlayMod(playMod);
        if(!pPlayMod) {
            continue;
        }

        Color& modColor = pPlayMod->GetSkinColor();
        if(modColor.GetAsUINTSwap() != 0xFFFFFFFF) {
            if(finalColor.GetAsUINTSwap() == m_skinColor.GetAsUINTSwap()) {
                finalColor.r = modColor.r;
                finalColor.g = modColor.g;
                finalColor.b = modColor.b;
                finalColor.a = modColor.a;
            }
            else {
                finalColor.r = (finalColor.r * modColor.r) / 255;
                finalColor.g = (finalColor.g * modColor.g) / 255;
                finalColor.b = (finalColor.b * modColor.b) / 255;
                finalColor.a = (finalColor.a * modColor.a) / 255;
            }
        }
    }

    return finalColor.GetAsUINTSwap();
}

void CharacterData::GetActiveModsForDialog(DialogBuilder& db)
{
    PlayModManager* pModMgr = GetPlayModManager();

    for(auto& modID : m_activePlayMods)
    {
        PlayMod* pPlayMod = pModMgr->GetPlayMod(modID);
        PlayerPlayModInfo* pPlayerMod = GetPlayMod(modID);
        if(!pPlayMod || !pPlayerMod)
        {
            db.AddLabelWithIcon("UNKNOWN (" + ToString(modID) + ")", ITEM_ID_BLANK);
            continue;
        }

        string label = pPlayMod->GetName();
        if(pPlayerMod->durationMS > 0)
        {
            uint64 elapsedMs = pPlayerMod->timer.GetElapsedTime();

            if(elapsedMs >= pPlayerMod->durationMS)
            {
                label += " `o(`wNONE `oleft)";
            }
            else
            {
                label += " `o(`w" + Time::ConvertTimeToStr(pPlayerMod->durationMS - elapsedMs) + " `oleft)";
            }
        }

        db.AddLabelWithIcon(label, pPlayMod->GetDisplayItem());
    }
}

bool CharacterData::HasPlayMod(ePlayModType modType)
{
    if(PlayModManager::IsRequiredUpdate(modType)) {
        for(auto& playMod : m_reqUpdatePlayMods) {
            if(playMod.modType == modType) {
                return true;
            }
        }
    }
    else {
        for(auto& playMod : m_staticPlayMods) {
            if(playMod.modType == modType) {
                return true;
            }
        }
    }

    return false;
}

PlayerPlayModInfo* CharacterData::GetPlayMod(ePlayModType modType)
{
    if(PlayModManager::IsRequiredUpdate(modType)) {
        for(auto& playMod : m_reqUpdatePlayMods) {
            if(playMod.modType == modType) {
                return &playMod;
            }
        }
    }
    else {
        for(auto& playMod : m_staticPlayMods) {
            if(playMod.modType == modType) {
                return &playMod;
            }
        }
    }

    return nullptr;
}

PlayMod* CharacterData::AddPlayMod(ePlayModType modType)
{
    if(modType == PLAYMOD_TYPE_NONE) {
        return nullptr;
    }

    if(HasPlayMod(modType)) {
        return nullptr;
    }

    PlayMod* pPlayMod = GetPlayModManager()->GetPlayMod(modType);
    if(!pPlayMod) {
        LOGGER_LOG_ERROR("Attempted to add playmod %d for but its not exists??", (int16)modType);
        return nullptr;
    }

    SetCharState(pPlayMod->GetCharFlags());
    
    if(pPlayMod->GetPunchType() > 0) {
        m_punchType = pPlayMod->GetPunchType();
    }

    m_punchDamage += pPlayMod->GetPunchDamage();
    m_speed += pPlayMod->GetSpeed();
    m_buildRange += pPlayMod->GetBuildRange();
    m_punchPower += pPlayMod->GetPunchPower();
    
    PlayerPlayModInfo playerMod;
    playerMod.modType = pPlayMod->GetType();

    if(PlayModManager::IsRequiredUpdate(pPlayMod->GetType())) {
        playerMod.durationMS = pPlayMod->GetTime() * 1000;
        m_reqUpdatePlayMods.push_back(std::move(playerMod));
    }
    else {
        m_staticPlayMods.push_back(std::move(playerMod));
    }

    m_activePlayMods.push_back(pPlayMod->GetType());
    SetNeededUpdates(pPlayMod);
    return pPlayMod;
}

PlayMod* CharacterData::RemovePlayMod(ePlayModType modType)
{
    PlayerPlayModInfo* pPlayerMod = GetPlayMod(modType);
    if(!pPlayerMod) {
        return nullptr;
    }

    PlayMod* pPlayMod = GetPlayModManager()->GetPlayMod(modType);
    if(!pPlayMod) {
        return nullptr;
    }

    RemoveCharState(pPlayMod->GetCharFlags());
    
    m_punchType = 0;
    m_punchDamage -= pPlayMod->GetPunchDamage();
    m_speed -= pPlayMod->GetSpeed();
    m_buildRange -= pPlayMod->GetBuildRange();
    m_punchPower -= pPlayMod->GetPunchPower();

    SetNeededUpdates(pPlayMod);
    RemovePlayMod(pPlayerMod);

    GetNextPunchType();
    return pPlayMod;
}

void CharacterData::RemovePlayMod(PlayerPlayModInfo* pPlayerMod)
{
    if(PlayModManager::IsRequiredUpdate(pPlayerMod->modType)) {
        if(pPlayerMod != &m_reqUpdatePlayMods.back()) {
            *pPlayerMod = std::move(m_reqUpdatePlayMods.back());
        }
        m_reqUpdatePlayMods.pop_back();
    }
    else {
        if(pPlayerMod != &m_staticPlayMods.back()) {
            *pPlayerMod = std::move(m_staticPlayMods.back());
        }
        m_staticPlayMods.pop_back();
    }

    for(auto& activeMod : m_activePlayMods) {
        if(activeMod == pPlayerMod->modType) {
            if(&activeMod != &m_activePlayMods.back()) {
                *(&activeMod) = std::move(m_activePlayMods.back());
            }
            m_activePlayMods.pop_back();
            break;
        } 
    }
}

void CharacterData::SetNeededUpdates(PlayMod* pPlayMod)
{
    if(pPlayMod->GetSkinColor().GetAsUINTSwap() != 0xFFFFFFFF) {
        m_needSkinUpdate = true;
    }

    if(
        pPlayMod->GetCharFlags() != 0 || pPlayMod->GetPunchType() != 0
    ) {
        m_needCharStateUpdate = true;
    }
}

void CharacterData::GetNextPunchType()
{
    PlayModManager* pPlayModMgr = GetPlayModManager();
    for(auto& activeMod : m_activePlayMods) {
        PlayMod* pPlayMod = pPlayModMgr->GetPlayMod(activeMod);
        if(!pPlayMod) {
            continue;
        }

        if(pPlayMod->GetPunchType() > 0) {
            m_punchType = pPlayMod->GetPunchType();
            m_needCharStateUpdate = true;
        }
    }
}
