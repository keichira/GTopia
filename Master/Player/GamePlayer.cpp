#include "GamePlayer.h"
#include "Database/Table/PlayerDBTable.h"
#include "../Context.h"
#include "Math/Random.h"
#include "IO/Log.h"

GamePlayer::GamePlayer(ENetPeer* pPeer)
: Player(pPeer)
{
}

GamePlayer::~GamePlayer()
{
}

void GamePlayer::OnHandleDatabase(QueryTaskResult&& result)
{    
    switch(m_state) {
        case PLAYER_STATE_LOGIN_CHECKING_ACCOUNT: {
            LoginCheckingAccount(std::move(result));
            break;
        }
    }

    result.Destroy();
}

void GamePlayer::StartLoginRequest(ParsedTextPacket<25> &packet)
{
    if(m_state != PLAYER_STATE_LOGIN_REQUEST) {
        SendLogonFailWithLog("");
        return;
    }

    if(!m_loginDetail.Serialize(packet, this, false)) {
        SendLogonFailWithLog("`4HUH?! ``Are you sure everything is alright?");
        return;
    }

    m_state = PLAYER_STATE_LOGIN_GETTING_ACCOUNT;
    LoginGetAccount();
}

void GamePlayer::LoginGetAccount()
{
    if(m_state != PLAYER_STATE_LOGIN_GETTING_ACCOUNT) {
        SendLogonFailWithLog("");
        return;
    }

    m_state = PLAYER_STATE_LOGIN_CHECKING_ACCOUNT;

    if(m_loginDetail.tankIDName.empty()) {
        QueryRequest req = MakePlayerByMacReq(m_loginDetail.mac, m_loginDetail.rid, m_loginDetail.platformType, GetNetID());
        DatabaseGetPlayerByMac(GetContext()->GetDatabasePool(), req);
    }
    else {

    }
}

void GamePlayer::LoginCheckingAccount(QueryTaskResult&& result)
{
    if(m_state != PLAYER_STATE_LOGIN_CHECKING_ACCOUNT) {
        SendLogonFailWithLog("");
        return;
    }

    if(!result.result) {
        SendLogonFailWithLog("`4OOPS! ``Something went wrong please re-connect");
        return;
    }

    if(result.result->GetRowCount() > 0) {
        m_userID = result.result->GetField("ID", 0).GetUINT();
    }
    else {
        QueryRequest req = MakePlayerCreateReq(
            m_loginDetail.requestedName, 
            m_loginDetail.platformType, 
            (uint16)RandomRangeInt(100, 999),
            m_loginDetail.mac,
            m_loginDetail.rid,
            GetNetID());

        DatabasePlayerCreate(GetContext()->GetDatabasePool(), req);
        m_state = PLAYER_STATE_CREATING_ACCOUNT;
    }

    m_state = PLAYER_STATE_SWITCHING_TO_GAME;
    SwitchingToGame();
}

void GamePlayer::CreatingAccount(QueryTaskResult&& result)
{
    if(m_state != PLAYER_STATE_CREATING_ACCOUNT) {
        SendLogonFailWithLog("");
        return;
    }

    if(result.increment == 0) {
        SendLogonFailWithLog("`4OOPS! ``Please re-connect");
        return;
    }

    m_userID = result.increment;

    m_state = PLAYER_STATE_SWITCHING_TO_GAME;
    SwitchingToGame();
}

void GamePlayer::SwitchingToGame()
{
    if(m_state != PLAYER_STATE_SWITCHING_TO_GAME) {
        SendLogonFailWithLog("");
        return;
    }

    
}
