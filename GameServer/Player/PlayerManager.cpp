#include "PlayerManager.h"
#include "GamePlayer.h"
#include "../Context.h"

PlayerManager* GetPlayerManager()
{
    return PlayerManager::GetInstance();
}

PlayerManager::PlayerManager()
{
}

PlayerManager::~PlayerManager()
{
    RemoveAllPlayers();
}

GamePlayer* PlayerManager::GetPlayerByNetID(uint32 netID)
{
    auto it = m_gamePlayers.find(netID);
    if(it != m_gamePlayers.end()) {
        return it->second;
    }

    return nullptr;
}

GamePlayer* PlayerManager::GetPlayerByUserID(uint32 userID)
{
    for(auto& [_, pPlayer] : m_gamePlayers) {
        if(!pPlayer) {
            continue;
        }

        if(pPlayer->GetUserID() == userID) {
            return pPlayer;
        }
    }

    return nullptr;
}

void PlayerManager::AddPlayer(GamePlayer* pPlayer)
{
    if(!pPlayer) {
        return;
    }

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

uint32 PlayerManager::GetPlayerCount()
{
    return m_gamePlayers.size();
}

void PlayerManager::UpdatePlayers()
{
    if(m_lastUpdateTime.GetElapsedTime() < TICK_INTERVAL) {
        return;
    }

    for(auto& [_, pPlayer] : m_gamePlayers) {
        if(!pPlayer) {
            continue;
        }

        if(!pPlayer->HasState(PLAYER_STATE_IN_GAME)) {
            // ?
            return;
        }

        if(!pPlayer->HasState(PLAYER_STATE_LOGGING_OFF)) {
            pPlayer->Update();

            if(pPlayer->GetLastDBSaveTime().GetElapsedTime() >= pPlayer->GetNextDBSaveTime()) {
                pPlayer->SaveToDatabase();
            }
        }

        if(pPlayer->HasState(PLAYER_STATE_DELETE)) {
            m_pendingDelete.push_back(pPlayer);
        }
    }

    for(auto& pPlayer : m_pendingDelete) {
        if(!GetPlayerByNetID(pPlayer->GetNetID())) {
            continue;
        }

        m_gamePlayers.erase(pPlayer->GetNetID());
    }

    m_pendingDelete.clear();
    m_lastUpdateTime.Reset();
}

void PlayerManager::SaveAllToDatabase()
{
    for(auto& [_, pPlayer] : m_gamePlayers) {
        if(!pPlayer) {
            continue;
        }

        pPlayer->SaveToDatabase();
    }
}
