#pragma once

#include "Precompiled.h"

struct PlayerSession
{
    uint32 userID;
    string name;
    uint32 loginToken;
    uint16 serverID;
    string ip;
    uint64 loginTime;
};

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

public:
    PlayerSession* GetSessionByID(uint32 userID);
    void CreateSession(const PlayerSession& session);
    void EndSessionByID(uint32 userID);
    void EndSessionsByServer(uint16 serverID);

    GamePlayer* GetPlayerByNetID(uint32 netID);
    void AddPlayer(GamePlayer* pPlayer);
    void RemovePlayer(uint32 netID);
    void RemoveAllPlayers();
    uint32 GetInGamePlayerCount();

private:
    std::unordered_map<uint32, PlayerSession> m_sessions;
    std::unordered_map<uint32, GamePlayer*> m_gamePlayers;
};

PlayerManager* GetPlayerManager();