#pragma once

#include "Precompiled.h"
#include "Server/ServerBase.h"

class GameServer : public ServerBase {
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
};

GameServer* GetGameServer();