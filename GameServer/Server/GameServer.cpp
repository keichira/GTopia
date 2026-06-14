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
#include "MasterBroadway.h"

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

static const uint32 SMALL_PACKET_SIZE = 80;
static const uint32 MED_PACKET_SIZE   = 800;
static const uint32 LARGE_PACKET_SIZE = 1500;
static const uint32 HUGE_PACKET_SIZE  = 400 * 1024;

static const uint32 SMALL_PACKET_COUNT = 5000;
static const uint32 MED_PACKET_COUNT   = 256;
static const uint32 LARGE_PACKET_COUNT = 128;
static const uint32 HUGE_PACKET_COUNT  = 10;

GameServer::GameServer()
{
    PacketPoolConfig cfg;
    cfg.smallPacketSize = SMALL_PACKET_SIZE;
    cfg.smallPoolSize = SMALL_PACKET_COUNT;

    cfg.medPacketSize = MED_PACKET_SIZE;
    cfg.medPoolSize = MED_PACKET_COUNT;

    cfg.largePacketSize = LARGE_PACKET_SIZE;
    cfg.largePoolSize = LARGE_PACKET_COUNT;

    cfg.hugePacketSize = HUGE_PACKET_SIZE;
    cfg.hugePoolSize = HUGE_PACKET_COUNT;

    gPacketPool.Init(cfg);
}

GameServer::~GameServer()
{
}

void GameServer::OnEventConnect(NetworkEvent& event)
{
    GamePlayer* pPlayer = new GamePlayer();
    pPlayer->SetNetID(event.netID);

    GetPlayerManager()->AddPlayer(pPlayer);

    char ipBuffer[16] = { 0 };
    if(GetIPStringFromHost(event.host, ipBuffer, sizeof(ipBuffer)) == 0)
    {
        pPlayer->SetAddress(ipBuffer);
    }
    else
    {
        pPlayer->SetAddress("0.0.0.0");
    }

    pPlayer->SetState(PLAYER_STATE_LOGIN_REQUEST);
    pPlayer->SendHelloPacket();
}

void GameServer::OnEventReceive(NetworkEvent& event)
{
    if(!event.pPacket)
        return;

    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(event.netID);
    if(!pPlayer)
    {
        gPacketPool.Release(event.pPacket);
        return;
    }

    PooledPacket* pPacket = event.pPacket;
    uint32 msgType = GetMessageTypeFromEnetPacket(pPacket->payload, pPacket->dataLength);

    switch(msgType) 
    {
        case NET_MESSAGE_GENERIC_TEXT:
        case NET_MESSAGE_GAME_MESSAGE: 
        {
            LOGGER_LOG_DEBUG("%s", GetTextFromEnetPacket(pPacket->payload, pPacket->dataLength));

            if(pPlayer->HasState(PLAYER_STATE_LOGIN_REQUEST))
            {
                ParsedTextPacket<35> packet;
                ParseTextPacket(GetTextFromEnetPacket(pPacket->payload, pPacket->dataLength), pPacket->dataLength - 4, packet);

                pPlayer->StartLoginRequest(packet);
            }
            else if(pPlayer->HasState(PLAYER_STATE_ENTERING_GAME)) 
            {
                ParsedTextPacket<40> packet;
                ParseTextPacket(GetTextFromEnetPacket(pPacket->payload, pPacket->dataLength), pPacket->dataLength - 4, packet);
            
                auto pAction = packet.Find("action"_hash);
                if(pAction) 
                {
                    uint32 packetType = HashString(pAction->value, pAction->size);

                    if(packetType == "refresh_item_data"_hash ||
                        packetType == "enter_game"_hash ||
                        packetType == "refresh_player_tribute_data"_hash ||
                        packetType == "quit"_hash
                    ) {
                        m_messagePacket.Dispatch(packetType, pPlayer, packet);   
                    }
                }
            }
            else if(pPlayer->HasState(PLAYER_STATE_IN_GAME)) 
            {
                ParsedTextPacket<40> packet;
                ParseTextPacket(GetTextFromEnetPacket(pPacket->payload, pPacket->dataLength), pPacket->dataLength - 4, packet);
            
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
                break;
            }

            GetWorldManager()->OnHandleGamePacket(event);
            break;
        }
    }

    gPacketPool.Release(pPacket);
}

void GameServer::OnEventDisconnect(NetworkEvent& event)
{
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(event.netID);
    if(!pPlayer)
    {
        gPacketPool.Release(event.pPacket);
        return;
    }

    pPlayer->LogOff(true, true, true, false);
    GetPlayerManager()->RemovePlayer(pPlayer->GetNetID());

    if(event.pPacket) 
    {
        gPacketPool.Release(event.pPacket);
    }
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

void GameServer::Update()
{
    if(!m_pENetServer)
        return;

    Context* pContext = GetContext();

    uint32 currentCpuPermille = pContext->GetPerfStats().netCpuPermille; 
    usize outgoingQueueSize = gPacketOutgoingQueue.size_approx();

    uint32 currentBurstLimit = 0;
    bool isPanicMode = false;
    
    EvaluateNetHealth(outgoingQueueSize, currentCpuPermille, currentBurstLimit, isPanicMode);

    if(pContext->IsShutting()) 
    {
        currentBurstLimit = 5000;
    }

    uint32 processedSends = 0;
    NetworkEvent outEvent;

    while(processedSends < currentBurstLimit && gPacketOutgoingQueue.try_dequeue(outEvent))
    {
        processedSends++;

        auto it = m_connectionMap.find(outEvent.netID);
        if(it == m_connectionMap.end() || !it->second) 
        {
            if(outEvent.pPacket)
            {
                gPacketPool.Release(outEvent.pPacket);
            }
            continue;
        }

        if(outEvent.shouldDisconnect)
        {
            enet_peer_disconnect(it->second, 0);
            m_connectionMap.erase(it);
            
            if(outEvent.pPacket)
            {
                gPacketPool.Release(outEvent.pPacket);
            }
            continue;
        }

        if(!outEvent.pPacket)
            continue;

        ENetPacket* pEnetPacket = nullptr;

        if(outEvent.isItemData)
        {
            GameUpdatePacket* pGamePacket = GetGamePacketFromEnetPacket(outEvent.pPacket->payload, outEvent.pPacket->dataLength, false);
            if(!pGamePacket || pGamePacket->type != NET_GAME_PACKET_SEND_ITEM_DATABASE_DATA)
                continue;

            ItemsClientData* pClientData = GetItemInfoManager()->GetClientData(pGamePacket->field_11, pGamePacket->field_10);
            if(!pClientData || (pClientData && !pClientData->pItemData))
                continue;

            pGamePacket->field_10 = 0;
            pGamePacket->field_11 = 0;
            
            pEnetPacket = enet_packet_create(nullptr, outEvent.pPacket->dataLength + pClientData->compressSize, ENET_PACKET_FLAG_RELIABLE);
            memcpy(pEnetPacket->data, outEvent.pPacket->payload, outEvent.pPacket->dataLength);
            memcpy(pEnetPacket->data + outEvent.pPacket->dataLength - 1, pClientData->pItemData, pGamePacket->extraDataSize);
        }
        else
        {
            pEnetPacket = enet_packet_create(outEvent.pPacket->payload, outEvent.pPacket->dataLength, ENET_PACKET_FLAG_RELIABLE);
        }

        if(pEnetPacket)
        {
            enet_peer_send(it->second, 0, pEnetPacket);
        }

        gPacketPool.Release(outEvent.pPacket);
    }

    if(pContext->IsShutting())
        return;

    uint32 processedReceives = 0;
    ENetEvent inEvent;
    
    uint32 recvLimit = isPanicMode ? gNetBurstConfig.panicBurst : currentBurstLimit;

    while(!pContext->IsShutting() && processedReceives < recvLimit && m_pENetServer->Update(&inEvent))
    {
        processedReceives++;
        if(!inEvent.peer)
            continue;

        switch(inEvent.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
            {
                uint32 newConnID = ++m_lastConnectionID; 
                if(newConnID == 0) newConnID = ++m_lastConnectionID;
            
                inEvent.peer->data = (void*)(uintptr_t)newConnID;            
                m_connectionMap[newConnID] = inEvent.peer;
            
                NetworkEvent netEvent;
                netEvent.type = ENET_EVENT_TYPE_CONNECT;
                netEvent.netID = newConnID;
                netEvent.host = inEvent.peer->address.host;
                m_networkQueue.enqueue(std::move(netEvent));
                break;
            }

            case ENET_EVENT_TYPE_RECEIVE:
            {
                if(!inEvent.packet || inEvent.packet->dataLength < 4)
                {
                    enet_packet_destroy(inEvent.packet);
                    continue;
                }

                uint32 packetLen = inEvent.packet->dataLength;
                uint32 msgType = GetMessageTypeFromEnetPacket(inEvent.packet->data, packetLen);

                if(msgType != NET_MESSAGE_GAME_MESSAGE && msgType != NET_MESSAGE_GAME_PACKET && msgType != NET_MESSAGE_GENERIC_TEXT)
                {
                    enet_packet_destroy(inEvent.packet);
                    continue;
                }

                // we can add more checks based on type

                if(msgType == NET_MESSAGE_GAME_PACKET && packetLen > SMALL_PACKET_SIZE)
                {
                    enet_packet_destroy(inEvent.packet);
                    continue;
                }

                if((msgType == NET_MESSAGE_GAME_MESSAGE || msgType == NET_MESSAGE_GENERIC_TEXT) && packetLen > MED_PACKET_SIZE)
                {
                    enet_packet_destroy(inEvent.packet);
                    continue;
                }

                PooledPacket* pPacket = gPacketPool.Acquire(packetLen);
                if(!pPacket)
                {
                    enet_packet_destroy(inEvent.packet);
                    continue;
                }

                pPacket->dataLength = packetLen;
                memcpy(pPacket->payload, inEvent.packet->data, packetLen);

                enet_packet_destroy(inEvent.packet);

                NetworkEvent netEvent{ ENET_EVENT_TYPE_RECEIVE, (uint32)(uintptr_t)inEvent.peer->data, pPacket };
                m_networkQueue.enqueue(std::move(netEvent));
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT:
            {
                if(!inEvent.peer)
                    continue;

                uint32 netID = (uint32)(uintptr_t)inEvent.peer->data;
                inEvent.peer->data = nullptr;

                m_connectionMap.erase(netID);

                NetworkEvent netEvent{ ENET_EVENT_TYPE_DISCONNECT, netID, nullptr };
                m_networkQueue.enqueue(std::move(netEvent));
                break;
            }
        }
    }
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