#pragma once

#include "Precompiled.h"
#include "Utils/Timer.h"

class GamePlayer;

class PlayerManager {
public:
    PlayerManager();
    ~PlayerManager();

public:
    static PlayerManager* GetInstance()
    {
        static PlayerManager instance;
        return &instance;
    }

    GamePlayer* GetPlayerByNetID(uint32 netID);
    GamePlayer* GetPlayerByUserID(uint32 userID);
    void AddPlayer(GamePlayer* pPlayer);
    void RemovePlayer(uint32 netID);
    void RemoveAllPlayers();
    uint32 GetPlayerCount();

    void UpdatePlayers();
    void SaveAllToDatabase();

private:
    std::unordered_map<uint32, GamePlayer*> m_gamePlayers;
    std::vector<GamePlayer*> m_pendingDelete;
    Timer m_lastUpdateTime;
};

PlayerManager* GetPlayerManager();