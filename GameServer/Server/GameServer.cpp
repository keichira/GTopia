#include "GameServer.h"
#include "Packet/NetPacket.h"
#include "Packet/GamePacket.h"
#include "../Player/GamePlayer.h"
#include "Packet/PacketUtils.h"
#include "IO/Log.h"

#include "../Event/UDP/GameMessage/RefreshItemData.h"
#include "../Event/UDP/GameMessage/EnterGame.h"
#include "../Event/UDP/GameMessage/JoinRequest.h"
#include "../Event/UDP/GameMessage/RefreshTributeData.h"

GameServer::GameServer()
{

}

GameServer::~GameServer()
{
    
}

void GameServer::OnEventConnect(ENetEvent& event)
{
    if(!event.peer) {
        return;
    }

    GamePlayer* pPlayer = new GamePlayer(event.peer);
    event.peer->data = pPlayer;

    m_playerCache.insert_or_assign(pPlayer->GetNetID(), pPlayer);

    pPlayer->SetState(PLAYER_STATE_LOGIN_REQUEST);
    pPlayer->SendHelloPacket();
}

void GameServer::OnEventReceive(ENetEvent& event)
{
    if(!event.peer) {
        return;
    }

    GamePlayer* pPlayer = (GamePlayer*)event.peer->data;
    if(event.peer != pPlayer->GetPeer()) {
        return;
    }

    uint32 msgType = GetMessageTypeFromEnetPacket(event.packet);
    LOGGER_LOG_DEBUG("%s", GetTextFromEnetPacket(event.packet));

    switch(msgType) {
        case NET_MESSAGE_GENERIC_TEXT: {
            
            switch(pPlayer->GetState()) {
                case PLAYER_STATE_LOGIN_REQUEST: {
                    ParsedTextPacket<25> packet;
                    ParseTextPacket(GetTextFromEnetPacket(event.packet), event.packet->dataLength - 4, packet);

                    pPlayer->StartLoginRequest(packet);
                    break;
                }

                case PLAYER_STATE_ENTERING_GAME: {
                    ParsedTextPacket<8> packet;
                    ParseTextPacket(GetTextFromEnetPacket(event.packet), event.packet->dataLength - 4, packet);
                
                    auto pAction = packet.Find(CompileTimeHashString("action"));
                    if(pAction) {
                        uint32 packetType = HashString(pAction->value, pAction->size);

                        if(
                            packetType == CompileTimeHashString("refresh_item_data") ||
                            packetType == CompileTimeHashString("enter_game") ||
                            packetType == CompileTimeHashString("refresh_player_tribute_data")
                        ) {
                            m_messagePacket.Dispatch(packetType, pPlayer, packet);   
                        }
                    }
                    break;
                }
            }

            break;
        }

        case NET_MESSAGE_GAME_MESSAGE: {
            ParsedTextPacket<8> packet;
            ParseTextPacket(GetTextFromEnetPacket(event.packet), event.packet->dataLength - 4, packet);
        
            auto pAction = packet.Find(CompileTimeHashString("action"));
            if(pAction) {
                uint32 packetType = HashString(pAction->value, pAction->size);
                m_messagePacket.Dispatch(packetType, pPlayer, packet);
            }
            break;
        }
    }
}

void GameServer::OnEventDisconnect(ENetEvent& event)
{
    if(!event.peer) {
        return;
    }

    GamePlayer* pPlayer = (GamePlayer*)event.peer->data;
    if(event.peer != pPlayer->GetPeer()) {
        return;
    }
    
    auto it = m_playerCache.find(pPlayer->GetNetID());
    if(it != m_playerCache.end()) {
        SAFE_DELETE(pPlayer);
        m_playerCache.erase(it);
    }
}

void GameServer::RegisterEvents()
{
    m_messagePacket.Register(
        CompileTimeHashString("refresh_item_data"),
        Delegate<GamePlayer*, ParsedTextPacket<8>&>::Create<&RefreshItemData::Execute>()
    );

    m_messagePacket.Register(
        CompileTimeHashString("enter_game"),
        Delegate<GamePlayer*, ParsedTextPacket<8>&>::Create<&EnterGame::Execute>()
    );

    m_messagePacket.Register(
        CompileTimeHashString("refresh_player_tribute_data"),
        Delegate<GamePlayer*, ParsedTextPacket<8>&>::Create<&RefreshTributeData::Execute>()
    );

    m_messagePacket.Register(
        CompileTimeHashString("join_request"),
        Delegate<GamePlayer*, ParsedTextPacket<8>&>::Create<&JoinRequest::Execute>()
    );
}

void GameServer::Kill()
{
    ServerBase::Kill();

    for(auto& [_, pPlayer] : m_playerCache) {
        SAFE_DELETE(pPlayer);
    }

    m_playerCache.clear();
}

GameServer* GetGameServer() { return GameServer::GetInstance(); }