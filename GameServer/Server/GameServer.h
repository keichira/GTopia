#pragma once

#include "../Player/GamePlayer.h"
#include "../Player/PlayerManager.h"
#include "Event/EventDispatcher.h"
#include "Server/ServerBase.h"

class GameMessage_DialogReturn;

class GameServer : public ServerBase
{
public:
    GameServer();
    ~GameServer();

public:
    static GameServer* GetInstance()
    {
        static GameServer instance;
        return &instance;
    }

public:
    void OnEventConnect(NetworkEvent& event) override;
    void OnEventReceive(NetworkEvent& event) override;
    void OnEventDisconnect(NetworkEvent& event) override;
    void RegisterEvents() override;
    void Kill() override;
    void UpdateGameLogic(uint64 maxTimeMS) override;
    void Update() override;

public:
    void ForceSaveEverything();

private:
    template <void (*Function)(GamePlayer*, ParsedTextPacket<38>&)> void RegisterMessagePacket(uint32 eventHash)
    {
        m_messagePacket.Register(eventHash, Delegate<GamePlayer*, ParsedTextPacket<38>&>::Create<Function>());
    }

private:
    EventDispatcher<uint32, GamePlayer*, ParsedTextPacket<38>&> m_messagePacket;

    Timer m_playersLastUpdateTime;
};

GameServer* GetGameServer();