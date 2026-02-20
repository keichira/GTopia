#pragma once

#include "Precompiled.h"
#include "Server/ServerBase.h"

struct PlayerSession
{
    uint32 userID;
    string name;
    uint32 loginToken;
    uint16 serverID;
};

class GameServer : public ServerBase {
public:
    typedef std::vector<PlayerSession*> PlayerSessionCache;

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
    PlayerSessionCache& GetPlayerSessionCache() { return m_sessionCache; }

private:
    PlayerSessionCache m_sessionCache;
};

GameServer* GetGameServer();