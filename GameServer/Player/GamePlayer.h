#pragma once

#include "Database/QueryUtils.h"
#include "Math/Random.h"
#include "Math/Rect.h"
#include "Player/PlayMod.h"
#include "Player/Player.h"
#include "Player/Role.h"
#include "PlayerPlayModController.h"
#include "PlayerProgress.h"
#include "PlayerTrade.h"
#include "Utils/Timer.h"

class TileInfo;

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

class GamePlayer : public Player
{
public:
    GamePlayer();
    ~GamePlayer();

    void Update();

    void SetState(ePlayerState state) { m_state |= state; }
    void RemoveState(ePlayerState state) { m_state &= ~state; }
    bool HasState(ePlayerState state) const { return (m_state & state) != 0; }

    void SetFlags(uint32 flags) { m_flags |= flags; }
    void SetFlag(ePlayerFlags flag) { m_flags |= flag; }
    bool HasFlag(ePlayerFlags flag) const { return (m_flags & flag) != 0; }

    void StartLoginRequest(ParsedTextPacket<40>& packet);
    void HandleCheckSession(VariantVector&& result);
    void TransferToGame();
    void SaveToDatabase();
    void LogOff(bool forceDelete, bool saveToDb, bool endSession, bool sendNetworkPackets = true);

    bool HasGrowID() const { return !m_loginDetail.tankIDPass.empty(); }
    void CheckLimitsForAccountCreation(bool fromDialog, const VariantVector& extraData = VariantVector{});

    static void OnEnterGameCheckAndSendToWorldIfPossibleCB(QueryTaskResult&& result);
    static void OnEnterGameCB(QueryTaskResult&& result);
    static void CheckAccountCreationLimitCB(QueryTaskResult&& result);
    static void AccountCreationNameExistsCB(QueryTaskResult&& result);
    static void CreateAccountFinalCB(QueryTaskResult&& result);

    void SetTargetJoinWorld(const string& worldName, const string& doorID = "");
    void SendEnterDoorPacket(Vector2Float doorWorldPos);
    void HandleRenderWorld(VariantVector&& result);
    void SendPositionToWorldPlayers();

    void SetJoiningWorld(bool joining) { m_joiningWorld = joining; }
    bool IsJoiningWorld() const { return m_joiningWorld; }

    void SetCurrentWorld(uint32 worldID) { m_currentWorldID = worldID; }
    uint32 GetCurrentWorld() const { return m_currentWorldID; }

    void SetWorldPos(float x, float y)
    {
        m_worldPos.x = x;
        m_worldPos.y = y;
    }
    Vector2Float& GetWorldPos() { return m_worldPos; }
    const Vector2Float& GetWorldPos() const { return m_worldPos; }
    Vector2Float GetWorldPosCenter();
    RectFloat GetPlayerWorldRect();

    void SetRespawnPos(float x, float y)
    {
        m_respawnPos.x = x;
        m_respawnPos.y = y;
    }
    Vector2Float& GetRespawnPos() { return m_respawnPos; }

    string GetDisplayName(bool checkWorld);
    string GetRawName();
    string GetSpawnData(bool local);
    string GetCountryData();

    void SetSearchName(const string& name) { m_searchName = name; }
    const string& GetSearchName() const { return m_searchName; }

    Role* GetRole() const { return m_pRole; }
    void SetRole(Role* pRole) { m_pRole = pRole; }

    void ToggleCloth(int32 itemID);
    void ToggleBattlePetLeash(bool forceFirstSlot);
    int32 GetActiveBattlePetSlot() const { return m_activeBattlePetSlot; }

    void SetGems(uint32 amount) { m_gems = amount; }
    int32 GetGems() const { return m_gems; }
    void SendGems(bool skipAnim);
    void ModifyGems(int32 count, bool sendToPlayer);

    void GiveXP(uint32 amount);
    uint32 GetPlayerLevel();
    uint32 GetPlayerNextLevelXP();

    void ModifyInventoryItem(int32 itemID, int16 amount);
    void TrashItem(int32 itemID, uint16 amount);
    void DropItem(int32 itemID, uint16 amount, bool openDialog);

    bool CanActivateItemNow() { return Time::GetSystemTime() - m_lastItemActivateTime >= 100; }
    void ResetItemActiveTime() { m_lastItemActivateTime = Time::GetSystemTime(); }

    void SendLockAccessRequest(GamePlayer* pOwner, TileInfo* pLockTile);
    void SetLockAccessTile(int32 lockIndex);
    TileInfo* GetLockAcessTile();
    int32 GetLockAcessOwnerID() const { return m_lockAccessOwnerID; }
    void AcceptLockAccess();

    float GetDistToTile(TileInfo* pGoalTile);
    uint32 GetDistToTileInTiles(TileInfo* pGoalTile);
    bool HasLOSToTile(TileInfo* pGoalTile);

    void SetGuestID(uint32 id) { m_guestID = id; }
    void OpenPaginatedDialog(std::unique_ptr<DialogPagination> newDialog);

    PlayerProgress& GetProgressData() { return m_progressData; }
    PlayerPlayModController& GetModController() { return m_modController; }
    PlayerTrade& GetTradeManager() { return m_tradeMgr; }

    Timer& GetLastActionTime() { return m_lastActionTime; }
    Timer& GetLastDBSaveTime() { return m_lastDbSaveTime; }
    Timer& GetLastJoinRequestTime() { return m_lastJoinRequestTime; }
    Timer& GetLastTileChangeTime() { return m_lastTileChangeTime; }
    Timer& GetLastSentAccessTime() { return m_lastSentAccessTime; }

    void RandomizeNextDBSaveTime() { m_nextDbSaveTime = RandomRangeInt(10 * 60, 15 * 60) * 1000; }
    uint64 GetNextDBSaveTime() const { return m_nextDbSaveTime; }

private:
    uint32 m_state;
    uint32 m_flags;
    Role* m_pRole;

    bool m_joiningWorld;
    uint32 m_currentWorldID;
    string m_targetJoinWorld;
    Vector2Float m_worldPos;
    Vector2Float m_respawnPos;

    string m_searchName;
    uint32 m_guestID;
    int32 m_gems;

    int32 m_lockAccessOwnerID;
    int32 m_lockAccessTileIndex;
    uint8 m_activeBattlePetSlot;
    uint64 m_lastItemActivateTime;

    PlayerProgress m_progressData;
    PlayerPlayModController m_modController;
    PlayerTrade m_tradeMgr;

    Timer m_lastSentAccessTime;
    Timer m_lastActionTime;
    Timer m_lastJoinRequestTime;
    Timer m_lastTileChangeTime;
    Timer m_lastDbSaveTime;
    Timer m_logonStartTime;
    uint64 m_nextDbSaveTime;
};