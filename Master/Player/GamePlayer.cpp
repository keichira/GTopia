#include "GamePlayer.h"
#include "Database/Table/PlayerDBTable.h"
#include "../Context.h"
#include "Math/Random.h"
#include "IO/Log.h"
#include "../Server/ServerManager.h"
#include "../Server/GameServer.h"
#include "Proton/ProtonUtils.h"
#include "PlayerManager.h"

GamePlayer::GamePlayer(ENetPeer* pPeer)
: Player(pPeer), m_state(PLAYER_STATE_IDLE)
{
}

GamePlayer::~GamePlayer()
{
}

void GamePlayer::StartLoginRequest(ParsedTextPacket<25>& packet)
{
    SetState(PLAYER_STATE_LOGIN_REQUEST);

    if(!m_loginDetail.Serialize(packet, this, false)) {
        SendLogonFailWithLog("`4HUH?! ``Are you sure everything is alright?");
        return;
    }

    LoginGetAccount();
}

void GamePlayer::LoginGetAccount()
{
    QueryRequest req;

    if(m_loginDetail.tankIDName.empty()) {
        if(m_loginDetail.platformType == Proton::PLATFORM_ID_IOS) {
            if(m_loginDetail.mac == "02:00:00:00:00:00") {
                req = PlayerDB::GetByVID(m_loginDetail.vid, m_loginDetail.platformType, GetNetID());
            }
            else {
                req = PlayerDB::GetByHash(m_loginDetail.hash, m_loginDetail.platformType, GetNetID());
            }
        }
        else if(m_loginDetail.platformType == Proton::PLATFORM_ID_ANDROID) {
            if(!m_loginDetail.gid.empty()) {
                req = PlayerDB::GetByGID(m_loginDetail.gid, m_loginDetail.platformType, GetNetID());
            }
            else {
                if(m_loginDetail.mac == "02:00:00:00:00:00") {
                    SendLogonFailWithLog("`4Unable to log on: ``Unfortunately your device has a Mac address of 02:00:00:00:00:00 which is invalid.");
                    return;
                }

                req = PlayerDB::GetByMac(m_loginDetail.mac, m_loginDetail.platformType, GetNetID());
            }
        }
        else {
            if(m_loginDetail.mac == "02:00:00:00:00:00") {
                SendLogonFailWithLog("`4Unable to log on: ``Unfortunately your device has a Mac address of 02:00:00:00:00:00 which is invalid.");
                return;
            }
    
            req = PlayerDB::GetByMac(m_loginDetail.mac, m_loginDetail.platformType, GetNetID());
        }
    }
    else {
        req = PlayerDB::GetByNameAndPass(m_loginDetail.tankIDName, m_loginDetail.tankIDPass, GetNetID());
    }

    req.callback = &GamePlayer::CheckAccountCB;

    DatabasePlayerExec(GetContext()->GetDatabasePool(), req);
}

void GamePlayer::CheckAccountCB(QueryTaskResult&& result)
{
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(result.ownerID);
    if(!pPlayer || !result.result) {
        pPlayer->SendLogonFailWithLog("`4OOPS! ``Something went wrong please re-connect");
        return;
    }

    if(result.result->GetRowCount() > 0) {
        Variant* pID = result.result->GetFieldSafe("ID", 0);

        if(!pID || pID->GetUINT() < 1) {
            pPlayer->SendLogonFailWithLog("`4OOPS! ``Something went wrong please re-connect");
            LOGGER_LOG_WARN("Got player but rows are null or id is not valid %s", pPlayer->GetAddress().c_str());
            return;
        }

        pPlayer->SetUserID(pID->GetUINT());
    }
    else {
        if(pPlayer->GetLoginDetail().tankIDName.empty()) {        
            QueryRequest req = PlayerDB::CountByIP(pPlayer->GetAddress(), pPlayer->GetNetID());

            req.callback = &GamePlayer::CheckCountOfIPCB;
            DatabasePlayerExec(GetContext()->GetDatabasePool(), req);
        }
        else {
            pPlayer->SendLogonFailWithLog("`4Unable to log on:`` That `wGrowID`` doesn't seem valid, or the password is wrong. If you don't have one, press `wCancel``, un-check `w'I have a GrowID'``, then click `wConnect``.");
            return;
        }

        return;
    }

    if(GetPlayerManager()->GetSessionByID(pPlayer->GetUserID())) { // TODO ALREADY ON
        pPlayer->SendLogonFailWithLog("`#You are still online, please wait few seconds and re-login again.");
        return;
    }

    QueryRequest req(pPlayer->GetNetID());
    req.callback = &GamePlayer::IdentifierUpdateResultCB;
    
    PlayerLoginDetail& loginDetail = pPlayer->GetLoginDetail();
    DatabasePlayerIdentifierExec(
        GetContext()->GetDatabasePool(),
        pPlayer->GetUserID(),
        loginDetail.mac, loginDetail.vid,
        loginDetail.sid, loginDetail.rid, loginDetail.gid,
        loginDetail.hash, req
    );
}

void GamePlayer::CheckCountOfIPCB(QueryTaskResult&& result)
{
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(result.ownerID);
    if(!pPlayer || !result.result) {
        pPlayer->SendLogonFailWithLog("`4OOPS! ``Something went wrong please re-connect");
        return;
    }

    if(result.result->GetRowCount() > GetContext()->GetGameConfig()->maxAccountsPerIP) {
        pPlayer->SendLogonFailWithLog("``Too many accounts created from this IP address (" + pPlayer->GetAddress() + "). `4Unable to create new account for guest.");
        return;
    }

    PlayerLoginDetail& loginDetail = pPlayer->GetLoginDetail();
    QueryRequest req = PlayerDB::Create(
        loginDetail.requestedName,
        loginDetail.platformType,
        RandomRangeInt(100, 999),
        loginDetail.mac,
        pPlayer->GetAddress(),
        pPlayer->GetNetID()
    );

    req.callback = &GamePlayer::CreateAccountCB;

    DatabasePlayerExec(GetContext()->GetDatabasePool(), req);
}

void GamePlayer::CreateAccountCB(QueryTaskResult&& result)
{
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(result.ownerID);
    if(!pPlayer) {
        return;
    }

    if(result.increment == 0) {
        pPlayer->SendLogonFailWithLog("`4OOPS! ``Please re-connect");
        return;
    }

    pPlayer->SetUserID(result.increment);

    QueryRequest req(pPlayer->GetNetID());
    req.callback = &GamePlayer::IdentifierUpdateResultCB;
    
    PlayerLoginDetail& loginDetail = pPlayer->GetLoginDetail();
    DatabasePlayerIdentifierExec(
        GetContext()->GetDatabasePool(),
        pPlayer->GetUserID(),
        loginDetail.mac, loginDetail.vid,
        loginDetail.sid, loginDetail.rid, loginDetail.gid,
        loginDetail.hash, req
    );
}

void GamePlayer::IdentifierUpdateResultCB(QueryTaskResult&& result)
{
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(result.ownerID);
    if(!pPlayer) {
        return;
    }

    pPlayer->SendToGame();
}

void GamePlayer::SendToGame()
{
    ServerInfo* pServer = GetServerManager()->GetBestGameServer();
    if(!pServer) {
        SendLogonFailWithLog("`4OOPS! ``Please re-connect");
        LOGGER_LOG_WARN("Tried to send player to game but the server is NULL?");
        return;
    }

    PlayerSession session;
    session.serverID = pServer->serverID;
    session.userID = m_userID;
    session.loginToken = RandomRangeInt(100000, 9999999);
    session.ip = m_address;

    GetPlayerManager()->CreateSession(session);
    SendOnSendToServer(pServer->port, session.loginToken, m_userID, pServer->wanIP);
    SetState(PLAYER_STATE_IDLE);
}
