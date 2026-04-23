#include "PlayerManager.h"
#include "GamePlayer.h"

PlayerManager* GetPlayerManager()
{
    return PlayerManager::GetInstance();
}

PlayerManager::PlayerManager()
{
}

PlayerManager::~PlayerManager()
{
    m_sessions.clear();
    RemoveAllPlayers();
}

PlayerSession* PlayerManager::GetSessionByID(uint32 userID)
{
    auto it = m_sessions.find(userID);
    if(it != m_sessions.end()) {
        return &it->second;
    }

    return nullptr;
}

void PlayerManager::CreateSession(const PlayerSession& session)
{
    m_sessions.insert_or_assign(session.userID, session);
}

void PlayerManager::EndSessionByID(uint32 userID)
{
    m_sessions.erase(userID);
}

void PlayerManager::EndSessionsByServer(uint16 serverID)
{
    for(auto it = m_sessions.begin(); it != m_sessions.end();) {
        if(it->second.serverID == serverID) {
            it = m_sessions.erase(it);
            continue;
        }

        ++it;
    }
}

GamePlayer* PlayerManager::GetPlayerByNetID(uint32 netID)
{
    auto it = m_gamePlayers.find(netID);
    if(it != m_gamePlayers.end()) {
        return it->second;
    }

    return nullptr;
}

void PlayerManager::AddPlayer(GamePlayer* pPlayer)
{
    m_gamePlayers.insert_or_assign(pPlayer->GetNetID(), pPlayer);
}

void PlayerManager::RemovePlayer(uint32 netID)
{
    GamePlayer* pPlayer = GetPlayerByNetID(netID);
    if(!pPlayer) {
        return;
    }

    SAFE_DELETE(pPlayer);
    m_gamePlayers.erase(netID);
}

void PlayerManager::RemoveAllPlayers()
{
    for(auto& [_, pPlayer] : m_gamePlayers) {
        SAFE_DELETE(pPlayer);
    }

    m_gamePlayers.clear();
}

uint32 PlayerManager::GetInGamePlayerCount()
{
    return m_gamePlayers.size();
}
