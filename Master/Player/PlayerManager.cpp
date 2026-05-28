#include "PlayerManager.h"
#include "GamePlayer.h"
#include "../Server/ServerManager.h"

PlayerManager* GetPlayerManager()
{
    return PlayerManager::GetInstance();
}

PlayerManager::PlayerManager()
: m_isPlayerCountDirty(false), m_totalPlayerCount(0)
{
}

PlayerManager::~PlayerManager()
{
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

    m_isPlayerCountDirty = true;
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
    if(!pPlayer) {
        return;
    }

    m_isPlayerCountDirty = true;
    m_gamePlayers.insert_or_assign(pPlayer->GetNetID(), pPlayer);
}

void PlayerManager::RemovePlayer(uint32 netID)
{
    auto it = m_gamePlayers.find(netID);
    if(it == m_gamePlayers.end())
        return;

    SAFE_DELETE(it->second);
    m_gamePlayers.erase(it);

    m_isPlayerCountDirty = true;
}

void PlayerManager::RemoveAllPlayers()
{
    for(auto& [_, pPlayer] : m_gamePlayers) 
    {
        SAFE_DELETE(pPlayer);
    }

    m_gamePlayers.clear();
}

uint32 PlayerManager::GetInGamePlayerCount()
{
    return m_gamePlayers.size();
}

uint32 PlayerManager::GetTotalPlayerCount()
{
    if(!m_isPlayerCountDirty) {
        return m_totalPlayerCount;
    }

    m_totalPlayerCount = GetServerManager()->GetPlayerCount() + GetInGamePlayerCount();
    return m_totalPlayerCount;
}
