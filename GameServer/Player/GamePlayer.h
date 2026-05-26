#pragma once

#include "Player/Player.h"
#include "Player/Role.h"
#include "Utils/Timer.h"
#include "Player/PlayMod.h"
#include "Database/QueryUtils.h"
#include "Math/Random.h"
#include "Math/Rect.h"
#include "PlayerProgress.h"
#include "PlayerPlayModController.h"

enum ePlayerState
{
    PLAYER_STATE_LOGIN_REQUEST = 1 << 0,
    PLAYER_STATE_ENTERING_GAME = 1 << 1,
    PLAYER_STATE_IN_GAME = 1 << 2,
    PLAYER_STATE_LOGGING_OFF = 1 << 3,
    PLAYER_STATE_RENDERING_WORLD = 1 << 4,
    PLAYER_STATE_DELETE = 1 << 5
};

enum ePlayerFlags
{
    PLAYER_FLAG_SUPPORTER = 1 << 0,
    PLAYER_FLAG_SUPER_SUPPORTER = 1 << 1
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

    void SetFlags(uint32 flags) { m_flags |= flags; }
    void SetFlag(ePlayerFlags flag) { m_flags |= flag; }
    bool HasFlag(ePlayerFlags flag) { return m_flags & flag; }

    void SetGems(uint32 amount) { m_gems = amount; }
    int32 GetGems() const { return m_gems; }
    void SendGems(bool skipAnim);
    void ModifyGems(int32 count, bool sendToPlayer);
    void GiveXP(uint32 amount);

    uint32 GetPlayerLevel();
    uint32 GetPlayerNextLevelXP();

    void StartLoginRequest(ParsedTextPacket<25>& packet);
    void HandleCheckSession(VariantVector&& result);
    void TransferToGame();

    void HandleRenderWorld(VariantVector&& result);

    void SaveToDatabase();
    void LogOff(bool forceDelete, bool saveToDb, bool endSession);

    void Update();

    void SetJoiningWorld(bool joining) { m_joiningWorld = joining; }
    bool IsJoiningWorld() { return m_joiningWorld; }
    
    void SetCurrentWorld(uint32 worldID) { m_currentWorldID = worldID; }
    uint32 GetCurrentWorld() const { return m_currentWorldID; }

    string GetDisplayName(bool checkWorld);
    string GetRawName();
    string GetSpawnData(bool local);

    void SetWorldPos(float x, float y) { m_worldPos.x = x; m_worldPos.y = y; }
    Vector2Float GetWorldPos() const { return m_worldPos; }
    RectFloat GetPlayerWorldRect();

    void SetRespawnPos(float x, float y) { m_respawnPos.x = x; m_respawnPos.y = y; }
    Vector2Float GetRespawnPos() const { return m_respawnPos; }

    Role* GetRole() const { return m_pRole; };
    void SetRole(Role* pRole) { m_pRole = pRole; }

    void SetGuestID(uint32 id) { m_guestID = id; }

    void ToggleCloth(uint16 itemID);

    void ModifyInventoryItem(uint16 itemID, int16 amount);
    void TrashItem(uint16 itemID, uint16 amount);
    void DropItem(uint16 itemID, uint16 amount, bool openDialog);

    bool CanActivateItemNow() { return Time::GetSystemTime() - m_lastItemActivateTime >= 100; };
    void ResetItemActiveTime() { m_lastItemActivateTime = Time::GetSystemTime(); }

    bool HasGrowID() { return !m_loginDetail.tankIDPass.empty(); }
    void CheckLimitsForAccountCreation(bool fromDialog, const VariantVector& extraData = VariantVector{});

    static void CheckAccountCreationLimitCB(QueryTaskResult&& result);
    static void AccountCreationNameExistsCB(QueryTaskResult&& result);
    static void CreateAccountFinalCB(QueryTaskResult&& result);

    void SendPositionToWorldPlayers();
    float GetDistToTile(TileInfo* pGoalTile);
    uint32 GetDistToTileInTiles(TileInfo* pGoalTile);

    Timer& GetLastActionTime() { return m_lastActionTime; }
    Timer& GetLastDBSaveTime() { return m_lastDbSaveTime; }
    Timer& GetLastJoinRequestTime() { return m_lastJoinRequestTime; }
    Timer& GetLastTileChangeTime() { return m_lastTileChangeTime; }

    void RandomizeNextDBSaveTime() { m_nextDbSaveTime = RandomRangeInt(10 * 60, 15 * 60) * 1000; };
    uint64 GetNextDBSaveTime() const { return m_nextDbSaveTime; };

    PlayerProgress& GetProgressData() { return m_progressData; }
    PlayerPlayModController& GetModController() { return m_modController; }

private:
    uint32 m_state;
    bool m_joiningWorld;
    uint32 m_currentWorldID;
    uint32 m_flags;
    int32 m_gems;

    PlayerProgress m_progressData;
    PlayerPlayModController m_modController;

    uint64 m_lastItemActivateTime;
    Timer m_lastActionTime;

    Timer m_lastJoinRequestTime;
    Timer m_lastTileChangeTime;

    uint32 m_guestID;

    Vector2Float m_respawnPos;
    Vector2Float m_worldPos;

    Timer m_lastDbSaveTime;
    uint64 m_nextDbSaveTime;

    Timer m_logonStartTime;
    Role* m_pRole;
};