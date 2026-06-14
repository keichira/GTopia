#include "PlayerPlayModController.h"
#include "GamePlayer.h"
#include "Utils/StringUtils.h"
#include "Utils/DialogBuilder.h"
#include "../World/WorldManager.h"

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

    if(ActivePlayMod* pExist = GetActiveMod(type))
    {
        pExist->remainingMS = pConfig->GetTime() * 1000;
        pExist->updateTimer.Reset();
        RecalculateStats();
        return pExist;
    }

    if(!pConfig->GetAddMessage().empty())
    {
        if(pConfig->GetTime() != 0)
        {
            m_pPlayer->SendOnConsoleMessage("`o" + pConfig->GetName() + " (`$" + pConfig->GetAddMessage() + " `omod added, `$" + Time::ConvertTimeToStr(pConfig->GetTime() * 1000) + "`oleft)");
        }
        else
        {
            m_pPlayer->SendOnConsoleMessage("`o" + pConfig->GetName() + " (`$" + pConfig->GetAddMessage() + " `omod added)");
        }
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
            if(pConfig && !pConfig->GetRemoveMessage().empty())
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
        charData.SetChar2State(pConfig->GetChar2States());
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

    if(HasPlayMod(PLAYMOD_TYPE_XENONITE))
    {
        if(World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_pPlayer->GetCurrentWorld()))
        {
            if(TileInfo* pTile = pWorld->GetTileManager()->GetKeyTile(KEY_TILE_XENONITE))
            {
                if(TileExtra_Xenonite* pXenoExtra = pTile->GetExtra<TileExtra_Xenonite>())
                {
                    bool needSendMessage = false;
                    string xenoMsg = "Xenonite has changed everyone's powers! ";

                    if(pXenoExtra->HasFlag(TILE_EXTRA_XENO_F_DOUBLE_JUMP)) 
                    {
                        if(!charData.HasCharState(CHAR_STATE_DOUBLE_JUMP)) 
                        {
                            charData.SetCharState(CHAR_STATE_DOUBLE_JUMP);
                            xenoMsg += "`2Double Jump granted!`` ";
                            needSendMessage = true;
                        }
                    } 
                    else if(pXenoExtra->HasFlag(TILE_EXTRA_XENO_B_DOUBLE_JUMP)) 
                    {
                        if(charData.HasCharState(CHAR_STATE_DOUBLE_JUMP))
                        {
                            charData.RemoveCharState(CHAR_STATE_DOUBLE_JUMP);
                            xenoMsg += "`6Double Jump blocked!`` ";
                            needSendMessage = true;
                        }
                    }

                    if(pXenoExtra->HasFlag(TILE_EXTRA_XENO_F_HIGH_JUMP)) 
                    {
                        if(700.0f < charData.gravity) 
                        {
                            charData.gravity = 700.0f;
                            xenoMsg += "`2High Jump granted!`` ";
                            needSendMessage = true;
                        }
                    } 
                    else if(pXenoExtra->HasFlag(TILE_EXTRA_XENO_B_HIGH_JUMP)) 
                    {
                        if(charData.gravity < 1000.0f) 
                        {
                            charData.gravity = 1000.0f;
                            xenoMsg += "`6High Jump blocked!`` ";
                            needSendMessage = true;
                        }
                    }

                    if(pXenoExtra->HasFlag(TILE_EXTRA_XENO_F_STRONG_PUNCH)) 
                    {
                        if(charData.punchPower < 500.0f) 
                        {
                            charData.punchPower = 500.0f;
                            xenoMsg += "`2Strong Punch granted!`` ";
                            needSendMessage = true;
                        }
                    } 
                    else if(pXenoExtra->HasFlag(TILE_EXTRA_XENO_B_STRONG_PUNCH)) 
                    {
                        if(200.0f < charData.punchPower) 
                        {
                            charData.punchPower = 200.0f;
                            xenoMsg += "`6Strong Punch blocked!`` ";
                            needSendMessage = true;
                        }
                    }

                    if(pXenoExtra->HasFlag(TILE_EXTRA_XENO_F_SPEED)) 
                    {
                        if(charData.accel < 310.0f) 
                        {
                            charData.accel = 310.0f;
                            xenoMsg += "`2Super Speed granted!`` ";
                            needSendMessage = true;
                        }
                    } 
                    else if(pXenoExtra->HasFlag(TILE_EXTRA_XENO_B_SPEED)) 
                    {
                        if(250.0f < charData.accel)
                        {
                            charData.accel = 250.0f;
                            xenoMsg += "`6Super Speed blocked!`` ";
                            needSendMessage = true;
                        }
                    }

                    if(pXenoExtra->HasFlag2(TILE_EXTRA_XENO_F_LONG_PUNCH)) 
                    {
                        if(charData.punchRange < 128)
                        {
                            charData.punchRange = 130;
                            xenoMsg += "`2Long Punch granted!`` ";
                            needSendMessage = true;
                        }
                    } 
                    else if(pXenoExtra->HasFlag2(TILE_EXTRA_XENO_B_LONG_PUNCH)) 
                    {
                        if(128 < charData.punchRange) 
                        {
                            charData.punchRange = 128;
                            xenoMsg += "`6Long Punch blocked!`` ";
                            needSendMessage = true;
                        }
                    }

                    if(pXenoExtra->HasFlag2(TILE_EXTRA_XENO_F_HEAT_RESIST)) 
                    {
                        if(0.5f < charData.fireDamage) 
                        {
                            charData.fireDamage = 0.5f;
                            xenoMsg += "`2Heat Resist granted!`` ";
                            needSendMessage = true;
                        }
                    } 
                    else if(pXenoExtra->HasFlag2(TILE_EXTRA_XENO_B_HEAT_RESIST)) 
                    {
                        if(charData.fireDamage < 1.0f) 
                        {
                            charData.fireDamage = 1.0f;
                            xenoMsg += "`6Heat Resist blocked!`` ";
                            needSendMessage = true;
                        }
                    }

                    if(pXenoExtra->HasFlag2(TILE_EXTRA_XENO_F_LONG_BUILD)) 
                    {
                        if(charData.buildRange < 128) 
                        {
                            charData.buildRange = 129;
                            xenoMsg += "`2Long Build granted!`` ";
                            needSendMessage = true;
                        }
                    } 
                    else if(pXenoExtra->HasFlag2(TILE_EXTRA_XENO_B_LONG_BUILD)) 
                    {
                        if(128 < charData.buildRange) 
                        {
                            charData.buildRange = 128;
                            xenoMsg += "`6Long Build blocked!`` ";
                            needSendMessage = true;
                        }
                    }

                    if(m_pPlayer && needSendMessage) 
                    {
                        m_pPlayer->SendOnTalkBubble(xenoMsg, true);
                    }
                }
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

        if(m_pPlayer && m_pPlayer->GetCharData().needCharStateUpdate)
        {
            if(mod.type == PLAYMOD_TYPE_XENONITE)
            {
                needRefresh = true;
            }
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
            RemovePlayMod(mod.type);
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
    if(mod.customTickTimer.GetElapsedTime() < 600)
        return;

    mod.customTickTimer.Reset();

    if(RandomRangeInt(0, 100) == 1)
    {
        m_pPlayer->ModifyInventoryItem(ITEM_ID_HAND_TORCH, -1);
        uint8 leftTorchCount = m_pPlayer->GetInventory().GetCountOfItem(ITEM_ID_HAND_TORCH);

        if(leftTorchCount == 0)
        {
            m_pPlayer->SendOnTalkBubble("`2My torch went out!", false);
            m_pPlayer->ToggleCloth(ITEM_ID_HAND_TORCH);
        }
        else
        {
            m_pPlayer->SendOnTalkBubble("`2My torch went out, i have " + ToString(leftTorchCount) + " more!", false);
        }
    }
}
