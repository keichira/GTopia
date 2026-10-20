#include "GameServer.h"
#include "../Context.h"
#include "../Player/GamePlayer.h"
#include "../Player/PlayerManager.h"
#include "IO/Log.h"
#include "Packet/GamePacket.h"
#include "Packet/NetPacket.h"
#include "Packet/PacketUtils.h"
#include "ServerManager.h"

static const uint32 SMALL_PACKET_SIZE = 0;
static const uint32 MED_PACKET_SIZE = 800;
static const uint32 LARGE_PACKET_SIZE = 0;
static const uint32 HUGE_PACKET_SIZE = 0;

static const uint32 SMALL_PACKET_COUNT = 0;
static const uint32 MED_PACKET_COUNT = 256;
static const uint32 LARGE_PACKET_COUNT = 0;
static const uint32 HUGE_PACKET_COUNT = 0;

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

GameServer::~GameServer() {}

void GameServer::OnEventConnect(NetworkEvent& event)
{
    GamePlayer* pPlayer = new GamePlayer();
    pPlayer->SetNetID(event.netID);

    GetPlayerManager()->AddPlayer(pPlayer);

    char ipBuffer[16] = {0};
    if (GetIPStringFromHost(event.host, ipBuffer, sizeof(ipBuffer)) == 0)
    {
        pPlayer->SetAddress(ipBuffer);
    }
    else
    {
        pPlayer->SetAddress("0.0.0.0");
    }
    pPlayer->SetAddressNum(event.host);

    pPlayer->SetState(PLAYER_STATE_LOGIN_REQUEST);
    pPlayer->SendHelloPacket();
}

void GameServer::OnEventReceive(NetworkEvent& event)
{
    if (!event.pPacket)
        return;

    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(event.netID);
    if (!pPlayer)
    {
        gPacketPool.Release(event.pPacket);
        return;
    }

    if (!GetServerManager()->HasAnyGameServer())
    {
        gPacketPool.Release(event.pPacket);
        pPlayer->SendLogonFailWithLog("`4OOPS! ``Unable to place you to a server, servers might be initializing.");
        return;
    }

    if (GetPlayerManager()->GetInGamePlayerCount() >= GetContext()->GetGameConfig()->maxLoginsAtOnce)
    {
        gPacketPool.Release(event.pPacket);
        pPlayer->SendLogonFailWithLog(
            "`4OOPS! ``Too many people logging in at once. Please press `5CANCEL`` and try again.");
        return;
    }

    PooledPacket* pPacket = event.pPacket;
    uint32 msgType = GetMessageTypeFromEnetPacket(pPacket->payload, pPacket->dataLength);

    switch (msgType)
    {
        case NET_MESSAGE_GENERIC_TEXT:
        {
            const char* textFromPacket = GetTextFromEnetPacket(pPacket->payload, pPacket->dataLength);
            LOGGER_LOG_DEBUG("%s", textFromPacket);

            switch (pPlayer->GetState())
            {
                case PLAYER_STATE_LOGIN_REQUEST:
                {
                    ParsedTextPacket<40> packet;
                    ParseTextPacket(GetTextFromEnetPacket(pPacket->payload, pPacket->dataLength),
                                    pPacket->dataLength - 4, packet);

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
    if (!pPlayer)
    {
        gPacketPool.Release(event.pPacket);
        return;
    }

    GetPlayerManager()->RemovePlayer(pPlayer->GetNetID());

    if (event.pPacket)
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
    if (!m_pENetServer)
        return;

    Context* pContext = GetContext();
    bool isShutting = GetContext()->IsShutting();
    bool needFlush = false;

    uint64 sendStartUS = 0;
    uint32 processedEvents = 0;
    uint32 processedSends = 0;
    NetworkEvent outEvent;

    while ((isShutting || !BudgetExpired(processedSends, sendStartUS)) && gPacketOutgoingQueue.try_dequeue(outEvent))
    {
        processedEvents++;

        if (!outEvent.targets.empty())
        {
            ENetPacket* pPacket = outEvent.pENetPacket;
            if (pPacket)
            {
                for (uint32 id : outEvent.targets)
                {
                    auto it = m_connectionMap.find(id);
                    if (it != m_connectionMap.end() && it->second.pPeer)
                    {
                        if (enet_peer_send(it->second.pPeer, 0, pPacket) == 0)
                            processedSends++;
                    }
                }
                if (pPacket->referenceCount == 0)
                    enet_packet_destroy(pPacket);
            }
            continue;
        }

        processedSends++;

        auto it = m_connectionMap.find(outEvent.netID);
        ENetPeer* pPeer = (it != m_connectionMap.end()) ? it->second.pPeer : nullptr;

        if (!pPeer)
        {
            if (outEvent.pENetPacket)
                enet_packet_destroy(outEvent.pENetPacket);

            if (outEvent.pPacket)
                gPacketPool.Release(outEvent.pPacket);
            continue;
        }

        if (outEvent.shouldDisconnect)
        {
            enet_peer_disconnect(pPeer, 0);
            m_connectionMap.erase(it);

            if (outEvent.pENetPacket)
                enet_packet_destroy(outEvent.pENetPacket);

            if (outEvent.pPacket)
                gPacketPool.Release(outEvent.pPacket);
            continue;
        }

        if (outEvent.pENetPacket)
        {
            if (enet_peer_send(pPeer, 0, outEvent.pENetPacket) != 0)
            {
                enet_packet_destroy(outEvent.pENetPacket);
            }
        }
        else if (outEvent.pPacket)
        {
            gPacketPool.Release(outEvent.pPacket);
        }
    }

    if (isShutting)
    {
        m_pENetServer->Flush();
        return;
    }

    m_pENetServer->Service();

    uint32 processedReceives = 0;
    uint64 recvStartUS = Time::GetTimeNowUS();
    uint64 nowMS = Time::GetSystemTime();
    ENetEvent inEvent;

    while (!BudgetExpired(processedReceives, recvStartUS) && m_pENetServer->Update(&inEvent))
    {
        processedReceives++;
        if (!inEvent.peer)
            continue;

        switch (inEvent.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
            {
                uint32 newConnID = ++m_lastConnectionID;
                if (newConnID == 0)
                    newConnID = ++m_lastConnectionID;

                inEvent.peer->data = (void*)(uintptr_t)newConnID;
                m_connectionMap[newConnID] = PeerConnection{inEvent.peer, nowMS, 0};

                NetworkEvent netEvent;
                netEvent.type = ENET_EVENT_TYPE_CONNECT;
                netEvent.netID = newConnID;
                netEvent.host = inEvent.peer->address.host;
                m_networkQueue.enqueue(std::move(netEvent));
                break;
            }

            case ENET_EVENT_TYPE_RECEIVE:
            {
                if (!inEvent.packet || inEvent.packet->dataLength < 4)
                {
                    enet_packet_destroy(inEvent.packet);
                    continue;
                }

                uint32 netID = (uint32)(uintptr_t)inEvent.peer->data;
                auto connIt = m_connectionMap.find(netID);
                if (connIt == m_connectionMap.end())
                {
                    enet_packet_destroy(inEvent.packet);
                    continue;
                }

                PeerConnection& conn = connIt->second;
                if (nowMS - conn.startMS >= 1000)
                {
                    conn.startMS = nowMS;
                    conn.packetCount = 0;
                }

                if (++conn.packetCount > 200)
                {
                    enet_packet_destroy(inEvent.packet);
                    if (conn.packetCount > 200 * 4)
                    {
                        KickPeer(netID, conn.pPeer);
                        needFlush = true;
                    }
                    continue;
                }

                uint32 packetLen = inEvent.packet->dataLength;
                uint32 msgType = GetMessageTypeFromEnetPacket(inEvent.packet->data, packetLen);

                PooledPacket* pPacket = gPacketPool.Acquire(packetLen);
                if (!pPacket)
                {
                    enet_packet_destroy(inEvent.packet);
                    continue;
                }

                if ((msgType == NET_MESSAGE_GAME_MESSAGE || msgType == NET_MESSAGE_GENERIC_TEXT) &&
                    packetLen > MED_PACKET_SIZE)
                {
                    enet_packet_destroy(inEvent.packet);
                    continue;
                }

                pPacket->dataLength = packetLen;
                memcpy(pPacket->payload, inEvent.packet->data, packetLen);

                enet_packet_destroy(inEvent.packet);

                NetworkEvent netEvent{ENET_EVENT_TYPE_RECEIVE, (uint32)(uintptr_t)inEvent.peer->data, nullptr, pPacket};
                m_networkQueue.enqueue(std::move(netEvent));
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT:
            {
                uint32 netID = (uint32)(uintptr_t)inEvent.peer->data;
                inEvent.peer->data = nullptr;

                m_connectionMap.erase(netID);

                NetworkEvent netEvent{ENET_EVENT_TYPE_DISCONNECT, netID, nullptr};
                m_networkQueue.enqueue(std::move(netEvent));
                break;
            }
        }
    }

    if (needFlush)
        m_pENetServer->Flush();
}

void GameServer::LockUserID(uint32 userID)
{
    m_lockedUserIDs.insert(userID);
}

void GameServer::UnLockUserID(uint32 userID)
{
    m_lockedUserIDs.erase(userID);
}

bool GameServer::IsUserLocked(uint32 userID)
{
    return m_lockedUserIDs.find(userID) != m_lockedUserIDs.end();
}

GameServer* GetGameServer()
{
    return GameServer::GetInstance();
}