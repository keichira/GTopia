#include "GamePlayer.h"
#include "../Server/MasterBroadway.h"
#include "../Context.h"
#include "IO/Log.h"
#include "Utils/Timer.h"
#include "Item/ItemInfoManager.h"

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
        
    }

    result.Destroy();
}

void GamePlayer::OnHandleTCP(VariantVector&& result)
{
    switch(m_state) {
        case PLAYER_STATE_CHECKING_SESSION: {
            CheckingLoginSession(std::move(result));
            break;
        }
    }
}

void GamePlayer::StartLoginRequest(ParsedTextPacket<25>& packet)
{
    if(m_state != PLAYER_STATE_LOGIN_REQUEST) {
        SendLogonFailWithLog("");
        return;
    }

    if(!m_loginDetail.Serialize(packet, this, true)) {
        SendLogonFailWithLog("`4HUH?! ``Are you sure everything is alright?");
        return;
    }

    m_state = PLAYER_STATE_CHECKING_SESSION;
    GetMasterBroadway()->SendCheckSessionPacket(GetNetID(), m_loginDetail.user, m_loginDetail.token, GetContext()->GetID());
}

void GamePlayer::CheckingLoginSession(VariantVector&& result)
{
    if(!result[1].GetBool()) {
        SendLogonFailWithLog("");
        return;
    }

    m_state = PLAYER_STATE_LOGIN_GETTING_ACCOUNT;
    /**
     * send mysql request to load player data
     * like inventory 
     * but not now
     */

    /** temp */ TransferingPlayerToGame();
}

void GamePlayer::TransferingPlayerToGame()
{
    string settings;
    settings += "proto=144"; /** search it what it effects in client */
    settings += "|server_tick" + ToString(Time::GetSystemTime());
    settings += "|choosemusic=audio/mp3/about_theme.mp3";
    settings += "|usingStoreNavigation=1";
    settings += "|enableInventoryTab=1";

    ItemsClientData itemData = GetItemInfoManager()->GetClientData(m_loginDetail.platformType);
    auto pGameConfig = GetContext()->GetGameConfig();

    SendWelcomePacket(itemData.hash, pGameConfig->cdnServer, pGameConfig->cdnPath, settings, 0);
}
