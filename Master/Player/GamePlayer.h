#pragma once

#include "Player/Player.h"
#include "Packet/PacketUtils.h"
#include "Database/QueryUtils.h"

enum ePlayerState
{
    PLAYER_STATE_IDLE,
    PLAYER_STATE_LOGIN_REQUEST,
};

class GamePlayer : public Player {
public:
    GamePlayer(ENetPeer* pPeer);
    ~GamePlayer();

public:
    void SetState(ePlayerState state) { m_state = state; }
    ePlayerState GetState() const { return m_state; }

    void StartLoginRequest(ParsedTextPacket<25>& packet);
    void LoginGetAccount();

    static void CheckAccountCB(QueryTaskResult&& result);
    static void CheckCountOfIPCB(QueryTaskResult&& result);
    static void CreateAccountCB(QueryTaskResult&& result);
    static void IdentifierUpdateResultCB(QueryTaskResult&& result);

    void SendToGame();

private:
    ePlayerState m_state;
};