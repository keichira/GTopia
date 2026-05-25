#pragma once

#include "Precompiled.h"
#include "Player/PlayModManager.h"
#include "Utils/Timer.h"

class GamePlayer;
class DialogBuilder;

struct ActivePlayMod 
{
    ePlayModType type = PLAYMOD_TYPE_NONE;
    uint32 remainingMS = 0;
    int32 extraData = 0;
    Timer updateTimer;
    Timer customTickTimer;
};

class PlayerPlayModController 
{
public:
    PlayerPlayModController(GamePlayer* pPlayer);
    ~PlayerPlayModController();

    void Update();
    
    ActivePlayMod* AddPlayMod(ePlayModType type);
    bool RemovePlayMod(ePlayModType type);
    bool HasPlayMod(ePlayModType type);
    ActivePlayMod* GetActiveMod(ePlayModType type);
    uint32 GetActiveModCount() const { return m_activeMods.size(); };

    void RecalculateStats();
    void BuildActiveModsDialog(DialogBuilder& db);

    uint32 GetSkinColor() const { return m_cachedSkinColor; }

private:
    void OnUpdateTorch(ActivePlayMod& mod);

private:
    GamePlayer* m_pPlayer;
    std::vector<ActivePlayMod> m_activeMods;
    uint32 m_cachedSkinColor;
};