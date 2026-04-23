#pragma once

#include "Player/Player.h"
#include "Player/Role.h"
#include "Utils/Timer.h"
#include "Player/PlayMod.h"
#include "Database/QueryUtils.h"
#include "Math/Random.h"

enum ePlayerState
{
    PLAYER_STATE_LOGIN_REQUEST = 1 << 0,
    PLAYER_STATE_ENTERING_GAME = 1 << 1,
    PLAYER_STATE_IN_GAME = 1 << 2,
    PLAYER_STATE_LOGGING_OFF = 1 << 3,
    PLAYER_STATE_RENDERING_WORLD = 1 << 4,
    PLAYER_STATE_DELETE = 1 << 5
};

class TileInfo;

class GamePlayer : public Player {
public:
    GamePlayer(ENetPeer* pPeer);
    ~GamePlayer();

public:
    void SetState(ePlayerState state) { m_state |= state; }
    void RemoveState(ePlayerState state) { m_state &= ~state; }
    bool HasState(ePlayerState state) const { return m_state & state; }

    void StartLoginRequest(ParsedTextPacket<25>& packet);
    void HandleCheckSession(VariantVector&& result);
    static void LoadAccountCB(QueryTaskResult&& result);
    void TransferToGame();

    void HandleRenderWorld(VariantVector&& result);

    void SaveToDatabase();
    void LogOff(bool forceDelete);

    void Update();

    void SetJoiningWorld(bool joining) { m_joiningWorld = joining; }
    bool IsJoiningWorld() { return m_joiningWorld; }
    
    void SetCurrentWorld(uint32 worldID) { m_currentWorldID = worldID; }
    uint32 GetCurrentWorld() const { return m_currentWorldID; }

    string GetDisplayName();
    string GetRawName();
    string GetSpawnData(bool local);

    void SetWorldPos(float x, float y) { m_worldPos.x = x; m_worldPos.y = y; }
    Vector2Float GetWorldPos() const { return m_worldPos; }

    void SetRespawnPos(float x, float y) { m_respawnPos.x = x; m_respawnPos.y = y; }
    Vector2Float GetRespawnPos() const { return m_respawnPos; }

    Role* GetRole() const { return m_pRole; };
    void SetRole(Role* pRole) { m_pRole = pRole; }

    void SetGuestID(uint32 id) { m_guestID = id; }

    void ToggleCloth(uint16 itemID);

    void ModifyInventoryItem(uint16 itemID, int16 amount);
    void TrashItem(uint16 itemID, uint8 amount);

    void AddPlayMod(ePlayModType modType, bool silent = false);
    void RemovePlayMod(ePlayModType modType, bool silent = false);
    void UpdateNeededPlayModThings();
    void UpdatePlayMods();
    void UpdateTorchPlayMod();

    bool CanActivateItemNow() { return Time::GetSystemTime() - m_lastItemActivateTime >= 100; };
    void ResetItemActiveTime() { m_lastItemActivateTime = Time::GetSystemTime(); }

    bool HasGrowID() { return !m_loginDetail.tankIDPass.empty(); }
    void CheckLimitsForAccountCreation(bool fromDialog, const VariantVector& extraData = VariantVector{});

    static void CheckAccountCreationLimitCB(QueryTaskResult&& result);
    static void AccountCreationNameExistsCB(QueryTaskResult&& result);
    static void CreateAccountFinalCB(QueryTaskResult&& result);

    void SendPositionToWorldPlayers();

    Timer& GetLastActionTime() { return m_lastActionTime; }
    Timer& GetLastDBSaveTime() { return m_lastDbSaveTime; }

    void RandomizeNextDBSaveTime() { m_nextDbSaveTime = RandomRangeInt(10 * 60, 15 * 60) * 1000; };
    uint64 GetNextDBSaveTime() const { return m_nextDbSaveTime; };

private:
    uint32 m_state;
    bool m_joiningWorld;
    uint32 m_currentWorldID;

    uint64 m_lastItemActivateTime;
    Timer m_lastActionTime;

    uint32 m_guestID;

    Vector2Float m_respawnPos;
    Vector2Float m_worldPos;

    Timer m_lastDbSaveTime;
    uint64 m_nextDbSaveTime;

    Timer m_logonStartTime;

    Role* m_pRole;
};