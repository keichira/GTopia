#include "MasterBroadway.h"
#include "IO/Log.h"
#include "Packet/NetPacket.h"
#include "Utils/Timer.h"
#include "GameServer.h"
#include "../World/WorldManager.h"
#include "../Context.h"

#include "../Event/TCP/TCPEventHello.h"
#include "../Event/TCP/TCPEventAuth.h"
#include "../Event/TCP/TCPEventPlayerSession.h"
#include "../Event/TCP/TCPEventWorldInit.h"
#include "../Event/TCP/TCPEventRenderWorld.h"
#include "../Event/TCP/TCPEventWorldSendPlayer.h"
#include "../Event/TCP/TCPEventKillServer.h"

MasterBroadway::MasterBroadway()
{
}

MasterBroadway::~MasterBroadway()
{
}

void MasterBroadway::RegisterEvents()
{
    ServerBroadwayBase::RegisterEvents();

    RegisterEvent<TCPEventHello>(TCP_PACKET_HELLO);
    RegisterEvent<TCPEventAuth>(TCP_PACKET_AUTH);
    RegisterEvent<TCPEventPlayerSession>(TCP_PACKET_PLAYER_CHECK_SESSION);
    RegisterEvent<TCPEventWorldInit>(TCP_PACKET_WORLD_INIT);
    RegisterEvent<TCPEventRenderWorld>(TCP_PACKET_RENDER_WORLD);
    RegisterEvent<TCPEventWorldSendPlayer>(TCP_PACKET_WORLD_SEND_PLAYER);
    RegisterEvent<TCPEventKillServer>(TCP_PACKET_KILL_SERVER);
}

void MasterBroadway::UpdateTCPLogic(uint64 maxTimeMS)
{
    uint64 startTime = Time::GetSystemTime();
    TCPPacketEvent event;

    while(m_packetQueue.try_dequeue(event)) {
        if(!event.pClient) {
            continue;
        }

        int8 type = event.data[0].GetINT();
        LOGGER_LOG_DEBUG("GOT TCP PACKET %d", type);

        m_events.Dispatch(type, event.pClient, event.data);

        if(Time::GetSystemTime() - startTime >= maxTimeMS) {
            break;
        }
    }
}

void MasterBroadway::SendHelloPacket()
{
    if(!m_connected || !m_pNetClient) {
        return;
    }

    VariantVector data(1);
    data[0] = TCP_PACKET_HELLO;

    m_pNetClient->Send(data);
}

void MasterBroadway::SendAuthPacket(const string& authKey)
{
    if(!m_connected || !m_pNetClient) {
        return;
    }

    VariantVector packet(4);
    packet[0] = TCP_PACKET_AUTH;

    /**
     * for now just send back the string
     * actually NetSocket was supporting TLS but removed it for now
     * planned to use HMAC for here for non-TLS socket but openssl lib is so big
     */
    packet[1] = authKey;
    packet[2] = (uint32)GetContext()->GetID();
    packet[3] = CONFIG_SERVER_GAME;

    m_pNetClient->Send(packet);
}

void MasterBroadway::SendCheckSessionPacket(int32 netID, uint32 userID, uint32 token, uint16 serverID)
{
    if(!m_connected || !m_pNetClient) {
        return;
    }

    VariantVector data(5);
    data[0] = TCP_PACKET_PLAYER_CHECK_SESSION;
    data[1] = netID;
    data[2] = userID;
    data[3] = token;
    data[4] = (uint32)serverID;

    m_pNetClient->Send(data);
}

void MasterBroadway::SendRenderWorldRequest(uint32 userID, uint32 worldID)
{
    if(!m_connected || !m_pNetClient) {
        return;
    }

    VariantVector data(4);
    data[0] = TCP_PACKET_RENDER_WORLD;
    data[1] = TCP_RENDER_REQUEST;
    data[2] = userID;
    data[3] = worldID;

    m_pNetClient->Send(data);
}

void MasterBroadway::SendWorldInitResult(bool succeed, uint32 worldID)
{
    if(!m_connected || !m_pNetClient) {
        return;
    }

    VariantVector data(3);
    data[0] = TCP_PACKET_WORLD_INIT;
    data[1] = succeed ? TCP_RESULT_OK : TCP_RESULT_FAIL;
    data[2] = worldID;

    m_pNetClient->Send(data);
}

void MasterBroadway::SendPlayerWorldJoin(int32 playerNetID, const string& worldName)
{
    if(!m_connected || !m_pNetClient) {
        return;
    }

    VariantVector data(4);
    data[0] = TCP_PACKET_WORLD_SEND_PLAYER;
    data[1] = (uint32)GetContext()->GetID();
    data[2] = playerNetID;
    data[3] = worldName;

    m_pNetClient->Send(data);
}

void MasterBroadway::SendHeartBeat()
{
    if(!m_connected || !m_pNetClient) {
        return;
    }

    VariantVector data(4);
    data[0] = TCP_PACKET_HEARTBEAT;
    data[1] = (uint32)GetContext()->GetID();
    data[2] = GetGameServer()->GetOnlineCount();
    data[3] = GetWorldManager()->GetWorldCount();

    m_pNetClient->Send(data);
    m_lastHearthBeatSentTime.Reset();
}

void MasterBroadway::SendEndPlayerSession(uint32 userID)
{
    if(!m_connected || !m_pNetClient) {
        return;
    }

    VariantVector data(2);
    data[0] = TCP_PACKET_PLAYER_END_SESSION;
    data[1] = userID;

    m_pNetClient->Send(data);   
}

void MasterBroadway::SendServerKillPacket()
{
    if(!m_connected || !m_pNetClient) {
        return;
    }

    VariantVector data(2);
    data[0] = TCP_PACKET_KILL_SERVER;
    data[1] = (uint32)GetContext()->GetID();

    m_pNetClient->Send(data);   
}

MasterBroadway* GetMasterBroadway() { return MasterBroadway::GetInstance(); }