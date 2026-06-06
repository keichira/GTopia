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
    void OnEventConnect(NetworkEvent& event) override;
    void OnEventReceive(NetworkEvent& event) override;
    void OnEventDisconnect(NetworkEvent& event) override;
    void Kill() override;
    void Update() override;
};

GameServer* GetGameServer();