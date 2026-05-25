#include "PlayerPlayModController.h"
#include "GamePlayer.h"
#include "Utils/StringUtils.h"
#include "Utils/DialogBuilder.h"

PlayerPlayModController::PlayerPlayModController(GamePlayer* pPlayer)
: m_pPlayer(pPlayer), m_cachedSkinColor(0xFFFFFFFF) 
{
}

PlayerPlayModController::~PlayerPlayModController() 
{
}

ActivePlayMod* PlayerPlayModController::AddPlayMod(ePlayModType type)
{
    if(type == PLAYMOD_TYPE_NONE) 
        return nullptr;

    PlayMod* pConfig = GetPlayModManager()->GetPlayMod(type);
    if(!pConfig)
        return nullptr;

    if(pConfig->GetTime() != 0)
    {
        m_pPlayer->SendOnConsoleMessage("`o" + pConfig->GetName() + " (`$" + pConfig->GetAddMessage() + " `omod added, `$" + Time::ConvertTimeToStr(pConfig->GetTime() * 1000) + "`oleft)");
    }
    else
    {
        m_pPlayer->SendOnConsoleMessage("`o" + pConfig->GetName() + " (`$" + pConfig->GetAddMessage() + " `omod added)");
    }

    if(ActivePlayMod* pExist = GetActiveMod(type))
    {
        pExist->remainingMS = pConfig->GetTime() * 1000;
        pExist->updateTimer.Reset();
        RecalculateStats();
        return pExist;
    }

    ActivePlayMod newMod;
    newMod.type = type;
    newMod.remainingMS = pConfig->GetTime() * 1000;
    newMod.updateTimer.Reset();
    newMod.customTickTimer.Reset();

    m_activeMods.push_back(std::move(newMod));
    
    RecalculateStats();
    return &m_activeMods.back();
}

bool PlayerPlayModController::RemovePlayMod(ePlayModType type)
{
    for(auto it = m_activeMods.begin(); it != m_activeMods.end(); ++it)
    {
        if(it->type == type)
        {
            if(&(*it) != &m_activeMods.back())
            {
                *it = std::move(m_activeMods.back());
            }
            m_activeMods.pop_back();

            PlayMod* pConfig = GetPlayModManager()->GetPlayMod(type);
            if(pConfig)
            {
                m_pPlayer->SendOnConsoleMessage("`o" + pConfig->GetName() + " (`$" + pConfig->GetRemoveMessage() + " `omod removed)");
            }
            
            RecalculateStats();
            return true;
        }
    }

    return false;
}

bool PlayerPlayModController::HasPlayMod(ePlayModType type)
{
    for(auto& mod : m_activeMods)
    {
        if(mod.type == type) 
            return true;
    }

    return false;
}

ActivePlayMod* PlayerPlayModController::GetActiveMod(ePlayModType type)
{
    for(auto& mod : m_activeMods)
    {
        if(mod.type == type) 
            return &mod;
    }
    return nullptr;
}

void PlayerPlayModController::RecalculateStats()
{
    CharacterData& charData = m_pPlayer->GetCharData();
    charData.ResetToBaseStats();

    PlayModManager* pModMgr = GetPlayModManager();
    Color finalColor = charData.skinColor;

    for(auto& activeMod : m_activeMods)
    {
        PlayMod* pConfig = pModMgr->GetPlayMod(activeMod.type);
        if(!pConfig) 
            continue;

        charData.SetCharState(pConfig->GetCharStates());
        if(pConfig->GetPunchType() > 0) 
        {
            charData.punchType = pConfig->GetPunchType();
        }

        charData.punchDamage += pConfig->GetPunchDamage();
        charData.speed += pConfig->GetSpeed();
        charData.buildRange += pConfig->GetBuildRange();
        charData.punchPower += pConfig->GetPunchPower();

        Color& modColor = pConfig->GetSkinColor();
        if(modColor.GetAsUINTSwap() != 0xFFFFFFFF)
        {
            if(finalColor.GetAsUINTSwap() == charData.skinColor.GetAsUINTSwap())
            {
                finalColor = modColor;
            }
            else
            {
                finalColor.r = (finalColor.r * modColor.r) / 255;
                finalColor.g = (finalColor.g * modColor.g) / 255;
                finalColor.b = (finalColor.b * modColor.b) / 255;
                finalColor.a = (finalColor.a * modColor.a) / 255;
            }
        }
    }

    m_cachedSkinColor = finalColor.GetAsUINTSwap();
    charData.needCharStateUpdate = true;
    charData.needSkinUpdate = true;
}

void PlayerPlayModController::Update()
{
    bool needRefresh = false;

    for(uint32 i = 0; i < m_activeMods.size(); )
    {
        ActivePlayMod& mod = m_activeMods[i];

        if(mod.type == PLAYMOD_TYPE_CARRYING_A_TORCH)
        {
            OnUpdateTorch(mod);
        }

        if(i >= m_activeMods.size() || m_activeMods[i].type != mod.type)
        {
            needRefresh = true;
            continue;
        }
        
        if(mod.remainingMS == 0)
        {
            ++i;
            continue;
        }

        uint64 elapsed = mod.updateTimer.GetElapsedTime();
        mod.updateTimer.Reset();

        if(elapsed >= mod.remainingMS)
        {
            m_activeMods[i] = std::move(m_activeMods.back());
            m_activeMods.pop_back();
            needRefresh = true;
        }
        else
        {
            mod.remainingMS -= elapsed;
            ++i;
        }
    }

    if(needRefresh)
    {
        RecalculateStats();
    }
}

void PlayerPlayModController::BuildActiveModsDialog(DialogBuilder& db)
{
    PlayModManager* pConfigMgr = GetPlayModManager();
    for(auto& mod : m_activeMods)
    {
        PlayMod* pConfig = pConfigMgr->GetPlayMod(mod.type);
        if(!pConfig) 
            continue;

        string label = pConfig->GetName();
        if(mod.remainingMS > 0)
        {
            label += " `o(`w" + Time::ConvertTimeToStr(mod.remainingMS) + " `oleft)";
        }
        db.AddLabelWithIcon(label, pConfig->GetDisplayItem());
    }
}

void PlayerPlayModController::OnUpdateTorch(ActivePlayMod& mod)
{
    if(mod.customTickTimer.GetElapsedTime() < 500)
        return;

    if(RandomRangeInt(0, 100) == 1)
    {
        m_pPlayer->ModifyInventoryItem(ITEM_ID_HAND_TORCH, -1);
        uint8 leftTorchCount = m_pPlayer->GetInventory().GetCountOfItem(ITEM_ID_HAND_TORCH);

        if(leftTorchCount == 0)
        {
            m_pPlayer->SendOnTalkBubble("`2My torch went out!", false);
            m_pPlayer->ToggleCloth(ITEM_ID_HAND_TORCH);

            RemovePlayMod(PLAYMOD_TYPE_CARRYING_A_TORCH);
        }
        else
        {
            m_pPlayer->SendOnTalkBubble("`2My torch went out, i have " + ToString(leftTorchCount) + " more!", false);
        }
    }
}
