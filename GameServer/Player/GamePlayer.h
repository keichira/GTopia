#pragma once

#include "Player/Player.h"
//#include "Role.h"

enum ePlayerState
{
    PLAYER_STATE_LOGIN_REQUEST,
    PLAYER_STATE_CHECKING_SESSION,
    PLAYER_STATE_LOGIN_GETTING_ACCOUNT,
    PLAYER_STATE_LOADING_ACCOUNT,
    PLAYER_STATE_ENTERING_GAME
};

class GamePlayer : public Player {
public:
    GamePlayer(ENetPeer* pPeer);
    ~GamePlayer();

public:
    void SetState(ePlayerState state) { m_state = state; }
    ePlayerState GetState() const { return m_state; }

    void OnHandleDatabase(QueryTaskResult&& result) override;
    void OnHandleTCP(VariantVector&& result) override;

    void StartLoginRequest(ParsedTextPacket<25>& packet);
    void CheckingLoginSession(VariantVector&& result);
    void TransferingPlayerToGame();

    void SetJoinWorld(const string& worldName) { m_joinWorldName = ToUpper(worldName); }
    bool IsJoiningWorld() { return m_joinWorldName != "EXIT"; }

    void SetCurrentWorld(const string& worldName) { m_currentWorldName = ToUpper(worldName); }

private:
    ePlayerState m_state;
    string m_joinWorldName;
    string m_currentWorldName;

    //Role* m_pRole;
};