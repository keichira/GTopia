#pragma once

#include "Precompiled.h"
#include "Server/ServerBase.h"

struct PlayerSession
{
    uint32 userID;
    string name;
    uint32 loginToken;
    uint16 serverID;
    string ip;
};

class GameServer : public ServerBase {
public:
    typedef std::unordered_map<uint32, PlayerSession*> PlayerSessionCache;

public:
    GameServer();
    ~GameServer();

public:
    static GameServer* GetInstance() {
        static GameServer instance;
        return &instance;
    }

public:
    void OnEventConnect(ENetEvent& event) override;
    void OnEventReceive(ENetEvent& event) override;
    void OnEventDisconnect(ENetEvent& event) override;
    void Kill() override;

public:
    PlayerSession* GetPlayerSessionByUserID(uint32 userID);
    void AddPlayerSession(PlayerSession* pSession);

private:
    PlayerSessionCache m_sessionCache;
};

GameServer* GetGameServer();