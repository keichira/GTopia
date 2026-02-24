#pragma once

#include "Player/Player.h"

enum ePlayerState
{
    PLAYER_STATE_LOGIN_REQUEST,
    PLAYER_STATE_CHECKING_SESSION,
    PLAYER_STATE_LOGIN_GETTING_ACCOUNT
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

private:
    ePlayerState m_state;
};