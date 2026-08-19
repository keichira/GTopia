#pragma once

#include "Memory/MemoryBuffer.h"
#include "Player/PlayModManager.h"
#include "Precompiled.h"
#include "Utils/Timer.h"

#define PLAYER_MOD_CONTROLLER_VERSION 1

class GamePlayer;
class DialogBuilder;

struct ActivePlayMod
{
    ePlayModType type = PLAYMOD_TYPE_NONE;
    uint32 timeValue = 0;
    int32 extraData = 0;
    Timer updateTimer;
    Timer customTickTimer;

    uint32 GetRemainingSeconds() const
    {
        if (timeValue == 0)
            return 0;

        if (IsOnlineOnlyPlayMod(type))
            return timeValue;

        uint32 nowSec = (uint32)(Time::GetTimeSinceEpoch());
        return (nowSec < timeValue) ? (timeValue - nowSec) : 0;
    }
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
    uint32 GetActiveModCount() const { return m_activeMods.size(); }

    void RecalculateStats();
    void BuildActiveModsDialog(DialogBuilder& db);

    uint32 GetMemEstimate();
    void Serialize(MemoryBuffer& memBuffer, bool write);
    void VerifyMods();

private:
    void OnUpdateTorch(ActivePlayMod& mod);

private:
    GamePlayer* m_pPlayer;
    std::vector<ActivePlayMod> m_activeMods;
};