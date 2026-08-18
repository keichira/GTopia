#include "GamePlayer.h"
#include "../Context.h"
#include "../Server/GameServer.h"
#include "../Server/ServerManager.h"
#include "Database/Table/PlayerDBTable.h"
#include "IO/Log.h"
#include "Math/Random.h"
#include "PlayerManager.h"
#include "Proton/ProtonUtils.h"

GamePlayer::GamePlayer() : m_state(PLAYER_STATE_IDLE), m_sessionName("`4<UNKNOWN>") {}

GamePlayer::~GamePlayer() {}

void GamePlayer::StartLoginRequest(ParsedTextPacket<40>& packet)
{
    SetState(PLAYER_STATE_LOGIN_REQUEST);

    m_loginStartTime.Reset();

    if (!m_loginDetail.Serialize(packet, this, false))
    {
        SendLogonFailWithLog("`4HUH?! ``Are you sure everything is alright?");
        return;
    }

    GameConfig* pGameConfig = GetContext()->GetGameConfig();

    float minVersion = 0.0f;
    float maxVersion = 0.0f;

    /*switch(m_loginDetail.platformType)
    {
        case Proton::PLATFORM_ID_WINDOWS:
        {
            minVersion = pGameConfig->windowsSupportedVersions[0];
            maxVersion = pGameConfig->windowsSupportedVersions[1];
            break;
        }

        case Proton::PLATFORM_ID_ANDROID:
        {
            minVersion = pGameConfig->androidSupportedVersions[0];
            maxVersion = pGameConfig->androidSupportedVersions[1];
            break;
        }

        case Proton::PLATFORM_ID_IOS:
        {
            minVersion = pGameConfig->iosSupportedVersions[0];
            maxVersion = pGameConfig->iosSupportedVersions[1];
            break;
        }

        case Proton::PLATFORM_ID_OSX:
        {
            minVersion = pGameConfig->macosSupportedVersions[0];
            maxVersion = pGameConfig->macosSupportedVersions[1];
            break;
        }
    }

    printf("%f %f\n", minVersion, maxVersion);

    if(m_loginDetail.gameVersion > maxVersion || m_loginDetail.gameVersion < minVersion)
    {
        SendLogonFailWithLog("`4Oops`o, your version is not supported");
        return;
    }*/

    LoginGetAccount();
}

void GamePlayer::LoginGetAccount()
{
    QueryRequest req;

    if (m_loginDetail.tankIDName.empty())
    {
        if (m_loginDetail.platformType == Proton::PLATFORM_ID_IOS)
        {
            if (m_loginDetail.mac == "02:00:00:00:00:00")
            {
                req = PlayerDB::GetByVID(m_loginDetail.vid, m_loginDetail.platformType, GetNetID());
            }
            else
            {
                req = PlayerDB::GetByHash(m_loginDetail.hash, m_loginDetail.platformType, GetNetID());
            }
        }
        else if (m_loginDetail.platformType == Proton::PLATFORM_ID_ANDROID)
        {
            if (!m_loginDetail.gid.empty())
            {
                req = PlayerDB::GetByGID(m_loginDetail.gid, m_loginDetail.platformType, GetNetID());
            }
            else
            {
                if (m_loginDetail.mac == "02:00:00:00:00:00")
                {
                    SendLogonFailWithLog("`4Unable to log on: ``Unfortunately your device has a Mac address of "
                                         "02:00:00:00:00:00 which is invalid.");
                    return;
                }

                req = PlayerDB::GetByMac(m_loginDetail.mac, m_loginDetail.platformType, GetNetID());
            }
        }
        else
        {
            if (m_loginDetail.mac == "02:00:00:00:00:00")
            {
                SendLogonFailWithLog("`4Unable to log on: ``Unfortunately your device has a Mac address of "
                                     "02:00:00:00:00:00 which is invalid.");
                return;
            }

            req = PlayerDB::GetByMac(m_loginDetail.mac, m_loginDetail.platformType, GetNetID());
        }
    }
    else
    {
        req = PlayerDB::GetByNameAndPass(m_loginDetail.tankIDName, m_loginDetail.tankIDPass, GetNetID());
    }

    req.callback = &GamePlayer::CheckAccountCB;
    DatabasePlayerExec(GetContext()->GetDatabasePool(), req);
}

void GamePlayer::CheckAccountCB(QueryTaskResult&& result)
{
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(result.ownerID);
    if (!pPlayer || !result.result)
    {
        pPlayer->SendLogonFailWithLog("`4OOPS! ``Something went wrong please re-connect");
        return;
    }

    if (result.result->GetRowCount() > 0)
    {
        Variant* pID = result.result->GetFieldSafe("ID", 0);

        if (!pID || pID->GetUINT() < 1)
        {
            pPlayer->SendLogonFailWithLog("`4OOPS! ``Something went wrong please re-connect");
            LOGGER_LOG_WARN("Got player but rows are null or ID is not valid");
            return;
        }

        PlayerLoginDetail& loginDetail = pPlayer->GetLoginDetail();
        if (!loginDetail.tankIDName.empty())
        {
            Variant* pName = result.result->GetFieldSafe("Name", 0);
            if (!pName || pName->GetString().empty())
            {
                pPlayer->SendLogonFailWithLog("`4OOPS! ``Something went wrong please re-connect");
                LOGGER_LOG_WARN("Got player but rows are null or Name is not valid");
                return;
            }

            pPlayer->SetSessionName(pName->GetString());
        }
        else
        {
            Variant* pGuestID = result.result->GetFieldSafe("GuestID", 0);
            if (!pGuestID || pGuestID->GetINT() < 100)
            {
                pPlayer->SendLogonFailWithLog("`4OOPS! ``Something went wrong please re-connect");
                LOGGER_LOG_WARN("Got guest player but rows are null or GuestID is not valid");
                return;
            }

            pPlayer->SetSessionName(loginDetail.requestedName + "_" + ToString(pGuestID->GetINT()));
        }

        pPlayer->SetUserID(pID->GetUINT());
    }
    else
    {
        if (pPlayer->GetLoginDetail().tankIDName.empty())
        {
            QueryRequest req = PlayerDB::CountByIP(pPlayer->GetAddress(), pPlayer->GetNetID());

            req.callback = &GamePlayer::CheckCountOfIPCB;
            DatabasePlayerExec(GetContext()->GetDatabasePool(), req);
        }
        else
        {
            pPlayer->SendLogonFailWithLog(
                "`4Unable to log on:`` That `wGrowID`` doesn't seem valid, or the password is wrong. If you don't have "
                "one, press `wCancel``, un-check `w'I have a GrowID'``, then click `wConnect``.");
            return;
        }

        return;
    }

    QueryRequest req(pPlayer->GetNetID());
    req.callback = &GamePlayer::IdentifierUpdateResultCB;

    PlayerLoginDetail& loginDetail = pPlayer->GetLoginDetail();
    DatabasePlayerIdentifierExec(GetContext()->GetDatabasePool(), pPlayer->GetUserID(), loginDetail.mac,
                                 loginDetail.vid, loginDetail.sid, loginDetail.rid, loginDetail.gid, loginDetail.hash,
                                 (loginDetail.tankIDName.empty()) ? loginDetail.requestedName : "", req);
}

void GamePlayer::CheckCountOfIPCB(QueryTaskResult&& result)
{
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(result.ownerID);
    if (!pPlayer || !result.result)
    {
        pPlayer->SendLogonFailWithLog("`4OOPS! ``Something went wrong please re-connect");
        return;
    }

    if (result.result->GetRowCount() > GetContext()->GetGameConfig()->maxAccountsPerIP)
    {
        pPlayer->SendLogonFailWithLog("``Too many accounts created from this IP address (" +
                                      string(pPlayer->GetAddress()) + "). `4Unable to create new account for guest.");
        return;
    }

    PlayerLoginDetail& loginDetail = pPlayer->GetLoginDetail();
    int32 guestID = RandomRangeInt(100, 999);

    pPlayer->SetSessionName(loginDetail.requestedName + "_" + ToString(guestID));

    QueryRequest req = PlayerDB::Create(loginDetail.requestedName, loginDetail.platformType, guestID, loginDetail.mac,
                                        pPlayer->GetAddress(), pPlayer->GetNetID());

    req.callback = &GamePlayer::CreateAccountCB;

    DatabasePlayerExec(GetContext()->GetDatabasePool(), req);
}

void GamePlayer::CreateAccountCB(QueryTaskResult&& result)
{
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(result.ownerID);
    if (!pPlayer)
    {
        return;
    }

    if (result.increment == 0)
    {
        pPlayer->SendLogonFailWithLog("`4OOPS! ``Please re-connect");
        return;
    }

    pPlayer->SetUserID(result.increment);

    QueryRequest req(pPlayer->GetNetID());
    req.callback = &GamePlayer::IdentifierUpdateResultCB;

    PlayerLoginDetail& loginDetail = pPlayer->GetLoginDetail();
    DatabasePlayerIdentifierExec(GetContext()->GetDatabasePool(), pPlayer->GetUserID(), loginDetail.mac,
                                 loginDetail.vid, loginDetail.sid, loginDetail.rid, loginDetail.gid, loginDetail.hash,
                                 (loginDetail.tankIDName.empty()) ? loginDetail.requestedName : "", req);
}

void GamePlayer::IdentifierUpdateResultCB(QueryTaskResult&& result)
{
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(result.ownerID);
    if (!pPlayer)
        return;

    CRASH_SET("LastPlayerUserID", pPlayer->GetUserID());
    pPlayer->SendToGame();
}

void GamePlayer::SendToGame()
{
    ServerInfo* pServer = nullptr;

    if (PlayerSession* pSession = GetPlayerManager()->GetSessionByID(m_userID))
    {
        pServer = GetServerManager()->GetServerByID(pSession->serverID);
    }
    else
    {
        pServer = GetServerManager()->GetBestGameServer();
    }

    if (!pServer)
    {
        SendLogonFailWithLog("`4OOPS! ``Please re-connect");
        LOGGER_LOG_WARN("Tried to send player to game but the server is NULL?");
        return;
    }

    PlayerSession session;
    session.serverID = pServer->serverID;
    session.userID = m_userID;
    session.loginToken = RandomRangeInt(100000, 9999999);
    session.ip = string(GetAddress());
    session.name = m_sessionName;

    GetPlayerManager()->CreateSession(session);
    SendOnSendToServer(pServer->port, session.loginToken, m_userID, pServer->wanIP, LOGON_MODE_WELCOME, "");
    SetState(PLAYER_STATE_IDLE);
}

void GamePlayer::Update()
{
    // todo here just template rn

    if (m_loginStartTime.GetElapsedTime() > 30000)
    {
        SendUDPDisconnectPacket(GetNetID());
        return;
    }
}