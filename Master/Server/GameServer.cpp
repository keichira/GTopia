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
    PacketPoolConfig cfg; //todo
    cfg.gamePacketSize = 0;
    cfg.gamePoolSize = 0;

    cfg.loginPacketSize = 800;
    cfg.loginPoolSize = 300;
    
    cfg.textPacketSize = 150;
    cfg.textPoolSize = 300;

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

    if(!GetServerManager()->HasAnyGameServer()) 
    {
        gPacketPool.Release(event.pPacket);
        pPlayer->SendLogonFailWithLog("`4OOPS! ``Unable to place you to a server, servers might be initializing.");
        return;
    }

    if(GetPlayerManager()->GetInGamePlayerCount() >= GetContext()->GetGameConfig()->maxLoginsAtOnce) 
    {
        gPacketPool.Release(event.pPacket);
        pPlayer->SendLogonFailWithLog("`4OOPS! ``Too many people logging in at once. Please press `5CANCEL`` and try again.");
        return;
    }

    PooledPacket* pPacket = event.pPacket;
    uint32 msgType = GetMessageTypeFromEnetPacket(pPacket->payload, pPacket->dataLength);

    switch(msgType) {
        case NET_MESSAGE_GENERIC_TEXT: {
            LOGGER_LOG_DEBUG("%s", GetTextFromEnetPacket(pPacket->payload, pPacket->dataLength));
            
            switch(pPlayer->GetState()) {
                case PLAYER_STATE_LOGIN_REQUEST: 
                {
                    ParsedTextPacket<35> packet;
                    ParseTextPacket(GetTextFromEnetPacket(pPacket->payload, pPacket->dataLength), pPacket->dataLength - 4, packet);

                    pPlayer->StartLoginRequest(packet);
                    break;
                }
            }

            break;
        }
    }

    gPacketPool.Release(event.pPacket);
}

void GameServer::OnEventDisconnect(NetworkEvent& event)
{
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(event.netID);
    if(!pPlayer)
    {
        gPacketPool.Release(event.pPacket);
        return;
    }

    GetPlayerManager()->RemovePlayer(pPlayer->GetNetID());

    if(event.pPacket) 
    {
        gPacketPool.Release(event.pPacket);
    }
}

void GameServer::Kill()
{
    ServerBase::Kill();
    GetPlayerManager()->RemoveAllPlayers();
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
        currentBurstLimit = 0xFFFFFFFF;
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
            
            if(outEvent.pPacket)
            {
                gPacketPool.Release(outEvent.pPacket);
            }
            continue;
        }

        if(outEvent.pPacket)
        {
            ENetPacket* pEnetPacket = enet_packet_create(outEvent.pPacket->payload, outEvent.pPacket->dataLength, ENET_PACKET_FLAG_RELIABLE);
            enet_peer_send(it->second, 0, pEnetPacket);
            gPacketPool.Release(outEvent.pPacket);
        }
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

                PooledPacket* pPacket = gPacketPool.Acquire(packetLen, false);
                if(!pPacket)
                    continue;

                pPacket->dataLength = packetLen;
                memcpy(pPacket->payload, inEvent.packet->data, packetLen);

                enet_packet_destroy(inEvent.packet);

                NetworkEvent netEvent{ ENET_EVENT_TYPE_RECEIVE, (uint32)(uintptr_t)inEvent.peer->data, pPacket };
                m_networkQueue.enqueue(std::move(netEvent));
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT:
            {
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

GameServer* GetGameServer() { return GameServer::GetInstance(); }