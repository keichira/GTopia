#include "GameServer.h"
#include "Packet/NetPacket.h"
#include "Packet/GamePacket.h"
#include "../Player/GamePlayer.h"
#include "Packet/PacketUtils.h"
#include "IO/Log.h"
#include "ServerManager.h"
#include "../Context.h"
#include "../Player/PlayerManager.h"

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

    GetPlayerManager()->AddPlayer(pPlayer);

    pPlayer->SetState(PLAYER_STATE_LOGIN_REQUEST);
    pPlayer->SendHelloPacket();
}

void GameServer::OnEventReceive(ENetEvent& event)
{
    if(!event.peer) {
        return;
    }

    GamePlayer* pPlayer = (GamePlayer*)event.peer->data;
    if(!pPlayer || event.peer != pPlayer->GetPeer()) {
        return;
    }

    if(!GetServerManager()->HasAnyGameServer()) 
    {
        pPlayer->SendLogonFailWithLog("`4OOPS! ``Unable to place you to a server, servers might be initializing.");
        return;
    }

    if(GetPlayerManager()->GetInGamePlayerCount() >= GetContext()->GetGameConfig()->maxLoginsAtOnce) 
    {
        pPlayer->SendLogonFailWithLog("`4OOPS! ``Too many people logging in at once. Please press `5CANCEL`` and try again.");
        return;
    }

    uint32 msgType = GetMessageTypeFromEnetPacket(event.packet);

    switch(msgType) {
        case NET_MESSAGE_GENERIC_TEXT: {
            LOGGER_LOG_DEBUG("%s", GetTextFromEnetPacket(event.packet));
            
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
    if(!pPlayer) {
        return;
    }

    event.peer->data = nullptr;
    GetPlayerManager()->RemovePlayer(pPlayer->GetNetID());
}

void GameServer::Kill()
{
    ServerBase::Kill();
    GetPlayerManager()->RemoveAllPlayers();
}

GameServer* GetGameServer() { return GameServer::GetInstance(); }