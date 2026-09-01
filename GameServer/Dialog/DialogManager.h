#pragma once

#include "Event/EventDispatcher.h"
#include "Packet/PacketUtils.h"
#include "Precompiled.h"

class GamePlayer;

class DialogManager
{
public:
    DialogManager() = default;
    ~DialogManager() = default;

public:
    static DialogManager* GetInstance()
    {
        static DialogManager instance;
        return &instance;
    }

public:
    void Execute(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
    void RegisterAllDialogs();

private:
    template <void (*DialogFunction)(GamePlayer*, ParsedTextPacket<38>&)> void RegisterDialog(uint32 eventHash)
    {
        m_dispatcher.Register(eventHash, Delegate<GamePlayer*, ParsedTextPacket<38>&>::Create<DialogFunction>());
    }

    EventDispatcher<uint32, GamePlayer*, ParsedTextPacket<38>&> m_dispatcher;
};

DialogManager* GetDialogManager();