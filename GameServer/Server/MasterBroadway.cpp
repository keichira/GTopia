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
#include "../Event/TCP/TCPEventHeartBeat.h"
#include "../Event/TCP/TCPEventCommand.h"

MasterBroadway::MasterBroadway()
: m_pNetClient(nullptr)
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
    RegisterEvent<TCPEventHeartBeat>(TCP_PACKET_HEARTBEAT);
    RegisterEvent<TCPEventCommand>(TCP_PACKET_ADMIN_COMMAND);
}

void MasterBroadway::UpdateTCPLogic(uint64 maxTimeMS)
{
    Timer startTime;
    TCPPacketEvent event;

    while(m_packetQueue.try_dequeue(event)) {
        if(!event.pClient || event.data.empty())
            continue;

        int32 type = event.data[0].GetINT();
        if(type != TCP_PACKET_HEARTBEAT) 
        {
            LOGGER_LOG_DEBUG("Received TCP Packet type %d", type);
        }

        m_events.Dispatch(type, event.pClient, event.data);

        if(startTime.GetElapsedTime() >= maxTimeMS)
            break;
    }
}

void MasterBroadway::SendHelloPacket()
{
    if(!m_pNetClient)
        return;

    VariantVector data(1);
    data[0] = TCP_PACKET_HELLO;

    m_pNetClient->Send(data);
}

void MasterBroadway::SendAuthPacket(const string& authKey)
{
    if(!m_pNetClient)
        return;

    VariantVector packet(4);
    packet[0] = TCP_PACKET_AUTH;

    /**
     * for now just send back the string
     * actually NetSocket was supporting TLS but removed it for now
     * planned to use HMAC for here for non-TLS socket but openssl lib is so huge
     */
    packet[1] = authKey;
    packet[2] = (uint32)GetContext()->GetID();
    packet[3] = CONFIG_SERVER_GAME;

    m_pNetClient->Send(packet);
}

void MasterBroadway::OnClientConnect(NetClient* pClient)
{
    if(!pClient)
        return;

    if(m_pNetClient && m_pNetClient != pClient) 
    {
        pClient->status = SOCKET_CLIENT_CLOSE;
        return;
    }

    m_pNetClient = pClient;
}

void MasterBroadway::OnClientDisconnect(NetClient* pClient)
{
    if(!pClient)
        return;

    if(m_pNetClient == pClient)
    {
        m_pNetClient = nullptr;
    }
}

void MasterBroadway::SendCheckSessionPacket(int32 netID, uint32 userID, uint32 token, uint16 serverID)
{
    if(!m_pNetClient)
        return;

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
    if(!m_pNetClient)
        return;

    VariantVector data(4);
    data[0] = TCP_PACKET_RENDER_WORLD;
    data[1] = TCP_RENDER_REQUEST;
    data[2] = userID;
    data[3] = worldID;

    m_pNetClient->Send(data);
}

void MasterBroadway::SendWorldInitResult(bool succeed, uint32 worldInstanceID)
{
    if(!m_pNetClient)
        return;

    VariantVector data(3);
    data[0] = TCP_PACKET_WORLD_INIT;
    data[1] = succeed ? TCP_RESULT_OK : TCP_RESULT_FAIL;
    data[2] = worldInstanceID;

    m_pNetClient->Send(data);
}

void MasterBroadway::SendPlayerWorldJoin(uint32 playerUserID, const string& worldName)
{
    if(!m_pNetClient)
        return;

    VariantVector data(3);
    data[0] = TCP_PACKET_WORLD_SEND_PLAYER;
    data[1] = playerUserID;
    data[2] = worldName;

    m_pNetClient->Send(data);
}

void MasterBroadway::SendHeartBeat()
{
    if(!m_pNetClient)
        return;

    VariantVector data(3);
    data[0] = TCP_PACKET_HEARTBEAT;
    data[1] = GetPlayerManager()->GetPlayerCount();
    data[2] = GetWorldManager()->GetWorldCount();

    m_pNetClient->Send(data);
    m_lastHearthBeatSentTime.Reset();
}

void MasterBroadway::SendEndPlayerSession(uint32 userID)
{
    if(!m_pNetClient)
        return;

    VariantVector data(2);
    data[0] = TCP_PACKET_PLAYER_END_SESSION;
    data[1] = userID;

    m_pNetClient->Send(data);   
}

void MasterBroadway::SendPlayerJoinedWorld(uint32 playerUserID, uint32 worldInstanceID)
{
    if(!m_pNetClient)
        return;

    VariantVector data(4);
    data[0] = TCP_PACKET_WORLD_PLAYER_SESSION;
    data[1] = TCP_WORLD_PLAYER_JOIN;
    data[2] = playerUserID;
    data[3] = worldInstanceID;

    m_pNetClient->Send(data); 
}

void MasterBroadway::SendPlayerLeftWorld(uint32 playerUserID, uint32 worldInstanceID)
{
    if(!m_pNetClient)
        return;

    VariantVector data(4);
    data[0] = TCP_PACKET_WORLD_PLAYER_SESSION;
    data[1] = TCP_WORLD_PLAYER_LEAVE;
    data[2] = playerUserID;
    data[3] = worldInstanceID;

    m_pNetClient->Send(data);   
}

bool MasterBroadway::Connect(const string& host, uint16 port, uint8 retryCount, const volatile sig_atomic_t* shutdownFlag)
{
    return ServerBroadwayBase::Connect(host, port, retryCount, &m_pNetClient, shutdownFlag);
}

void MasterBroadway::SendServerKillPacket()
{
    if(!m_pNetClient)
        return;

    VariantVector data(1);
    data[0] = TCP_PACKET_KILL_SERVER;

    m_pNetClient->Send(data);   
}

MasterBroadway* GetMasterBroadway() { return MasterBroadway::GetInstance(); }