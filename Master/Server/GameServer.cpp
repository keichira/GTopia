#include "GameServer.h"
#include "Packet/NetPacket.h"
#include "Packet/GamePacket.h"
#include "../Player/GamePlayer.h"
#include "Packet/PacketUtils.h"
#include "IO/Log.h"

GameServer::GameServer()
{
}

GameServer::~GameServer()
{
    Kill();
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

    switch(msgType) {
        case NET_MESSAGE_GENERIC_TEXT: {
            
            switch(pPlayer->GetState()) {
                case PLAYER_STATE_LOGIN_REQUEST: {
                    ParsedTextPacket<25> packet;
                    ParseTextPacket(GetTextFromEnetPacket(event.packet), event.packet->dataLength - 4, packet);

                    pPlayer->StartLoginRequest(packet);
                    break;
                }
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

void GameServer::Kill()
{
    ServerBase::Kill();
}

PlayerSession* GameServer::GetPlayerSessionByUserID(uint32 userID)
{
    auto it = m_sessionCache.find(userID);
    if(it != m_sessionCache.end()) {
        return it->second;
    }

    return nullptr;
}

void GameServer::AddPlayerSession(PlayerSession* pSession)
{
    m_sessionCache.insert_or_assign(pSession->userID, pSession);
}

GameServer* GetGameServer() { return GameServer::GetInstance(); }