#pragma once

#include "Server/ServerBase.h"
#include "Event/EventDispatcher.h"
#include "../Player/GamePlayer.h"

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
    void RegisterEvents() override;
    void Kill() override;

private:
    EventDispatcher<uint32, GamePlayer*, ParsedTextPacket<8>&> m_messagePacket;
};

GameServer* GetGameServer();