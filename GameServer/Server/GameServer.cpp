#include "GameServer.h"
#include "Packet/NetPacket.h"
#include "Packet/GamePacket.h"
#include "../Player/GamePlayer.h"
#include "Packet/PacketUtils.h"
#include "IO/Log.h"
#include "../World/WorldManager.h"
#include "Item/ItemInfoManager.h"
#include "Player/RoleManager.h"
#include "../Context.h"
#include "UserCacheManager.h"

#include "../Event/UDP/GameMessage/RefreshItemData.h"
#include "../Event/UDP/GameMessage/EnterGame.h"
#include "../Event/UDP/GameMessage/JoinRequest.h"
#include "../Event/UDP/GameMessage/RefreshTributeData.h"
#include "../Event/UDP/GameMessage/Input.h"
#include "../Event/UDP/GameMessage/QuitToExit.h"
#include "../Event/UDP/GameMessage/DialogReturn.h"
#include "../Event/UDP/GameMessage/Trash.h"
#include "../Event/UDP/GameMessage/GrowID.h"
#include "../Event/UDP/GameMessage/Quit.h"
#include "../Event/UDP/GameMessage/SetSkin.h"
#include "../Event/UDP/GameMessage/Drop.h"
#include "../Event/UDP/GameMessage/Wrench.h"
#include "../Event/UDP/GameMessage/Buy.h"
#include "../Event/UDP/GameMessage/Store.h"

#include "../Command/RenderWorld.h"
#include "../Command/GiveItem.h"
#include "../Command/Ghost.h"
#include "../Command/TogglePlayMod.h"
#include "../Command/Magic.h"
#include "../Command/AgeWorld.h"
#include "../Command/Emotes.h"

GameServer::GameServer()
{
}

GameServer::~GameServer()
{
}

void GameServer::OnEventConnect(ENetEvent& event)
{
    if(!event.peer)
        return;

    GamePlayer* pPlayer = new GamePlayer(event.peer);
    event.peer->data = pPlayer;

    GetPlayerManager()->AddPlayer(pPlayer);

    pPlayer->SetState(PLAYER_STATE_LOGIN_REQUEST);
    pPlayer->SendHelloPacket();
}

void GameServer::OnEventReceive(ENetEvent& event)
{
    if(!event.peer)
        return;

    GamePlayer* pPlayer = (GamePlayer*)event.peer->data;
    if(!pPlayer || event.peer != pPlayer->GetPeer())
        return;

    uint32 msgType = GetMessageTypeFromEnetPacket(event.packet);

    switch(msgType) 
    {
        case NET_MESSAGE_GENERIC_TEXT:
        case NET_MESSAGE_GAME_MESSAGE: 
        {
            LOGGER_LOG_DEBUG("%s", GetTextFromEnetPacket(event.packet));

            if(pPlayer->HasState(PLAYER_STATE_LOGIN_REQUEST))
            {
                ParsedTextPacket<30> packet;
                ParseTextPacket(GetTextFromEnetPacket(event.packet), event.packet->dataLength - 4, packet);

                pPlayer->StartLoginRequest(packet);
                return;
            }
            else if(pPlayer->HasState(PLAYER_STATE_ENTERING_GAME)) 
            {
                ParsedTextPacket<8> packet;
                ParseTextPacket(GetTextFromEnetPacket(event.packet), event.packet->dataLength - 4, packet);
            
                auto pAction = packet.Find("action"_hash);
                if(pAction) 
                {
                    uint32 packetType = HashString(pAction->value, pAction->size);

                    if(
                        packetType == "refresh_item_data"_hash ||
                        packetType == "enter_game"_hash ||
                        packetType == "refresh_player_tribute_data"_hash ||
                        packetType == "quit"_hash
                    ) {
                        m_messagePacket.Dispatch(packetType, pPlayer, packet);   
                    }
                }
                return;
            }
            else if(pPlayer->HasState(PLAYER_STATE_IN_GAME)) 
            {
                ParsedTextPacket<8> packet; // increase it for dialog_return?
                ParseTextPacket(GetTextFromEnetPacket(event.packet), event.packet->dataLength - 4, packet);
            
                auto pAction = packet.Find("action"_hash);
                if(pAction) 
                {
                    uint32 packetType = HashString(pAction->value, pAction->size);
                    m_messagePacket.Dispatch(packetType, pPlayer, packet);
                }
            }

            break;
        }

        case NET_MESSAGE_GAME_PACKET: 
        {
            if(pPlayer->GetCurrentWorld() == 0) 
            {
                /**
                 * response
                 */
                return;
            }

            GetWorldManager()->OnHandleGamePacket(event);
            break;
        }
    }
}

void GameServer::OnEventDisconnect(ENetEvent& event)
{
    if(!event.peer)
        return;

    GamePlayer* pPlayer = (GamePlayer*)event.peer->data;
    if(!pPlayer)
        return;

    pPlayer->LogOff(true, true, true);

    event.peer->data = nullptr;
    GetPlayerManager()->RemovePlayer(pPlayer->GetNetID());
}

void GameServer::RegisterEvents()
{
    RegisterMessagePacket<RefreshItemData>("refresh_item_data"_hash);
    RegisterMessagePacket<EnterGame>("enter_game"_hash);
    RegisterMessagePacket<RefreshTributeData>("refresh_player_tribute_data"_hash);
    RegisterMessagePacket<JoinRequest>("join_request"_hash);
    RegisterMessagePacket<Input>("input"_hash);
    RegisterMessagePacket<QuitToExit>("quit_to_exit"_hash);
    RegisterMessagePacket<DialogReturn>("dialog_return"_hash);
    RegisterMessagePacket<Trash>("trash"_hash);
    RegisterMessagePacket<GrowID>("growid"_hash);
    RegisterMessagePacket<Quit>("quit"_hash);
    RegisterMessagePacket<SetSkin>("setSkin"_hash);
    RegisterMessagePacket<Drop>("drop"_hash);
    RegisterMessagePacket<Wrench>("wrench"_hash);
    RegisterMessagePacket<Buy>("buy"_hash);
    RegisterMessagePacket<Store>("store"_hash);

    RegisterCommand<RenderWorld>();
    RegisterCommand<GiveItem>();
    RegisterCommand<Ghost>();
    RegisterCommand<TogglePlayMod>();
    RegisterCommand<Magic>();
    RegisterCommand<AgeWorld>();
    RegisterCommand<Emotes>();
}

void GameServer::UpdateGameLogic(uint64 maxTimeMS)
{
    ServerBase::UpdateGameLogic(maxTimeMS);
    GetPlayerManager()->UpdatePlayers();
    GetWorldManager()->UpdateWorlds();
    GetUserCacheManager()->Update();
}

void GameServer::ExecuteCommand(GamePlayer* pPlayer, std::vector<string>& args)
{
    if(!pPlayer || args.empty())
        return;

    uint32 hashCmd = HashString(args[0].substr(1));
    if(!m_commands.HasHandler(hashCmd)) 
    {
        pPlayer->SendOnConsoleMessage("`4Unknown command. ``Enter `$/help`` for a list of valid commands.");
        return;
    }

    m_commands.Dispatch(
        hashCmd,
        pPlayer, args
    );
}

void GameServer::ForceSaveEverything()
{
    GetPlayerManager()->SaveAllToDatabase();
    GetWorldManager()->SaveAllToDatabase();
}

void GameServer::Kill()
{
    ServerBase::Kill();

    GetItemInfoManager()->Kill();
    GetRoleManager()->Kill();
    GetWorldManager()->Kill();
    GetPlayerManager()->RemoveAllPlayers();
}

GameServer* GetGameServer() { return GameServer::GetInstance(); }