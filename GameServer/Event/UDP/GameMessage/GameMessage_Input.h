#pragma once

#include "../../../Player/GamePlayer.h"
#include "Event/EventDispatcher.h"
#include "Packet/PacketUtils.h"

class DialogReturn
{
public:
    DialogReturn();
    void Execute(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);

private:
    void RegisterDialogs();

    template <void (*DialogFunction)(GamePlayer*, ParsedTextPacket<38>&)> void RegisterDialog(uint32 eventHash)
    {
        m_dispatcher.Register(eventHash, Delegate<GamePlayer*, ParsedTextPacket<38>&>::Create<DialogFunction>());
    }

    EventDispatcher<uint32, GamePlayer*, ParsedTextPacket<38>&> m_dispatcher;
};

static DialogReturn dialogReturnMgr = DialogReturn();

void GameMessage_DialogReturn(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
void GameMessage_SetSkin(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
void GameMessage_Input(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);
void GameMessage_Wrench(GamePlayer* pPlayer, ParsedTextPacket<38>& packet);