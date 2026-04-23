#pragma once

#include "Server/ServerBase.h"
#include "Event/EventDispatcher.h"
#include "../Player/GamePlayer.h"
#include "../Command/CommandBase.h"
#include "../Player/PlayerManager.h"

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
    void UpdateGameLogic(uint64 maxTimeMS) override;

public:
    void ExecuteCommand(GamePlayer* pPlayer, std::vector<string>& args);

    void ForceSaveEverything();

private:
    template<class T>
    void RegisterMessagePacket(uint32 eventHash)
    {
        m_messagePacket.Register(
            eventHash,
            Delegate<GamePlayer*, ParsedTextPacket<8>&>::Create<&T::Execute>()
        );
    }

    template<class T>
    void RegisterCommand()
    {
        for(auto& alias : T::GetInfo().aliases) {
            m_commands.Register(
                alias,
                Delegate<GamePlayer*, std::vector<string>&>::Create<&T::Execute>()
            );
        }
    }

private:
    EventDispatcher<uint32, GamePlayer*, ParsedTextPacket<8>&> m_messagePacket;
    EventDispatcher<uint32, GamePlayer*, std::vector<string>&> m_commands;

    Timer m_playersLastUpdateTime;
};

GameServer* GetGameServer();