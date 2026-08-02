#include "MasterBroadway.h"
#include "../Context.h"
#include "../Server/GamePresenceManager.h"
#include "../World/WorldManager.h"
#include "GameServer.h"
#include "IO/Log.h"
#include "Packet/NetPacket.h"
#include "Utils/Timer.h"

#include "../Event/TCP/TCPEvent_Command.h"
#include "../Event/TCP/TCPEvent_Player.h"
#include "../Event/TCP/TCPEvent_Server.h"
#include "../Event/TCP/TCPEvent_World.h"

MasterBroadway::MasterBroadway() : m_pNetClient(nullptr), m_authState(BROADWAY_AUTH_NONE) {}

MasterBroadway::~MasterBroadway() {}

void MasterBroadway::RegisterEvents()
{
    ServerBroadwayBase::RegisterEvents();

    RegisterEvent<TCPEvent_Hello>(TCP_PACKET_HELLO);
    RegisterEvent<TCPEvent_Auth>(TCP_PACKET_AUTH);
    RegisterEvent<TCPEvent_PlayerCheckSession>(TCP_PACKET_PLAYER_CHECK_SESSION);
    RegisterEvent<TCPEvent_WorldInit>(TCP_PACKET_WORLD_INIT);
    RegisterEvent<TCPEvent_RenderWorld>(TCP_PACKET_RENDER_WORLD);
    RegisterEvent<TCPEvent_WorldSendPlayer>(TCP_PACKET_WORLD_SEND_PLAYER);
    RegisterEvent<TCPEvent_KillServer>(TCP_PACKET_KILL_SERVER);
    RegisterEvent<TCPEvent_HeartBeat>(TCP_PACKET_HEARTBEAT);
    RegisterEvent<TCPEvent_Command>(TCP_PACKET_COMMAND);
}

void MasterBroadway::UpdateTCPLogic(uint64 maxTimeMS)
{
    Timer startTime;
    TCPPacketEvent event;

    while (m_packetQueue.try_dequeue(event))
    {
        if (!event.pClient)
            continue;

        if (event.packetType != TCP_PACKET_HEARTBEAT)
        {
            LOGGER_LOG_DEBUG("Received TCP Packet %d (IsRaw: %d)", event.packetType, event.isRaw ? 1 : 0);
        }

        if (event.isRaw)
        {
            if (event.packetType == TCP_PACKET_PLAYER_SUBSCRIBE || event.packetType == TCP_PACKET_PLAYER_UNSUBSCRIBE ||
                event.packetType == TCP_PACKET_WORLD_SNAPSHOT || event.packetType == TCP_PACKET_WORLD_UPDATE ||
                event.packetType == TCP_PACKET_WORLD_REMOVE)
            {
                GetGamePresenceManager()->OnTCPPacket(event.packetType, event.rawData);
            }
        }
        else if (!event.data.empty())
        {
            m_events.Dispatch(event.packetType, event.pClient, event.data);
        }

        if (startTime.GetElapsedTime() >= maxTimeMS)
            break;
    }
}

void MasterBroadway::SendHelloPacket()
{
    if (!m_pNetClient)
        return;

    VariantVector data(1);
    data[0] = TCP_PACKET_HELLO;

    m_pNetClient->Send(data);
}

void MasterBroadway::SendAuthPacket(const string& authKey)
{
    if (!m_pNetClient)
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
    if (!pClient)
        return;

    if (m_pNetClient && m_pNetClient != pClient)
    {
        pClient->status = SOCKET_CLIENT_CLOSE;
        return;
    }

    m_pNetClient = pClient;
}

void MasterBroadway::OnClientDisconnect(NetClient* pClient)
{
    if (!pClient)
        return;

    if (m_pNetClient == pClient)
    {
        m_pNetClient = nullptr;
    }
}

void MasterBroadway::SendCheckSessionPacket(int32 netID, uint32 userID, uint32 token, uint16 serverID)
{
    if (!m_pNetClient)
        return;

    VariantVector data(5);
    data[0] = TCP_PACKET_PLAYER_CHECK_SESSION;
    data[1] = netID;
    data[2] = userID;
    data[3] = token;
    data[4] = (uint32)serverID;

    m_pNetClient->Send(data);
}

void MasterBroadway::SendRenderWorldRequest(uint32 userID, uint32 worldInstanceID)
{
    if (!m_pNetClient)
        return;

    VariantVector data(4);
    data[0] = TCP_PACKET_RENDER_WORLD;
    data[1] = TCP_RENDER_REQUEST;
    data[2] = userID;
    data[3] = worldInstanceID;

    m_pNetClient->Send(data);
}

void MasterBroadway::SendWorldInitResult(bool succeed, uint32 worldInstanceID)
{
    if (!m_pNetClient)
        return;

    VariantVector data(3);
    data[0] = TCP_PACKET_WORLD_INIT;
    data[1] = succeed ? TCP_RESULT_OK : TCP_RESULT_FAIL;
    data[2] = worldInstanceID;

    m_pNetClient->Send(data);
}

void MasterBroadway::SendPlayerWorldJoin(uint32 playerUserID, const string& worldName, const string& doorID)
{
    if (!m_pNetClient)
        return;

    VariantVector data(4);
    data[0] = TCP_PACKET_WORLD_SEND_PLAYER;
    data[1] = playerUserID;
    data[2] = worldName;
    data[3] = doorID;

    m_pNetClient->Send(data);
}

void MasterBroadway::SendHeartBeat()
{
    if (!m_pNetClient)
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
    if (!m_pNetClient)
        return;

    VariantVector data(2);
    data[0] = TCP_PACKET_PLAYER_END_SESSION;
    data[1] = userID;

    m_pNetClient->Send(data);
}

void MasterBroadway::SendPlayerJoinedWorld(uint32 playerUserID, uint32 worldInstanceID)
{
    if (!m_pNetClient)
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
    if (!m_pNetClient)
        return;

    VariantVector data(4);
    data[0] = TCP_PACKET_WORLD_PLAYER_SESSION;
    data[1] = TCP_WORLD_PLAYER_LEAVE;
    data[2] = playerUserID;
    data[3] = worldInstanceID;

    m_pNetClient->Send(data);
}

void MasterBroadway::SendPlayerPresenceSubscribe(const std::vector<uint32>& ids)
{
    if (!m_pNetClient || ids.empty())
        return;

    uint32 totalByteSize = (uint32)(ids.size() * sizeof(uint32));
    m_pNetClient->Send(TCP_PACKET_PLAYER_SUBSCRIBE, ids.data(), totalByteSize);
}

void MasterBroadway::SendPlayerPresenceUnsubscribe(const std::vector<uint32>& ids)
{
    if (!m_pNetClient || ids.empty())
        return;

    uint32 totalByteSize = (uint32)(ids.size() * sizeof(uint32));
    m_pNetClient->Send(TCP_PACKET_PLAYER_UNSUBSCRIBE, ids.data(), totalByteSize);
}

void MasterBroadway::SendRawPacket(eTCPPacketType type, void* pData, uint32 size)
{
    if (!m_pNetClient || !pData || size == 0)
        return;

    m_pNetClient->Send(type, pData, size);
}

bool MasterBroadway::ConnectAndAuth(const string& host, uint16 port, uint8 maxConnectAttempts,
                                    const volatile sig_atomic_t* shutdownFlag)
{
    if (!Connect(host, port, maxConnectAttempts, shutdownFlag))
    {
        LOGGER_LOG_ERROR("Initial TCP connection to Master Server failed.");
        return false;
    }

    SetAuthState(BROADWAY_AUTH_NONE);
    SendHelloPacket();
    LOGGER_LOG_INFO("Sent Hello packet. Waiting for Master response...");

    uint64 authStartTime = Time::GetSystemTime();

    while (GetAuthState() == BROADWAY_AUTH_NONE && (!shutdownFlag || *shutdownFlag == 0))
    {
        if (m_pNetSocket)
        {
            Update(true);
            UpdateTCPLogic(1);
        }

        if (!IsConnected())
        {
            LOGGER_LOG_ERROR("Master server closed the connection during auth!");
            break;
        }

        if (Time::GetSystemTime() - authStartTime >= 10000)
        {
            LOGGER_LOG_ERROR("Auth timeout.");
            break;
        }

        SleepMS(10);
    }

    if (GetAuthState() == BROADWAY_AUTH_SUCCESS)
    {
        LOGGER_LOG_INFO("Successfully authenticated with Master Server!");
        return true;
    }
    else if (GetAuthState() == BROADWAY_AUTH_FAILED)
    {
        LOGGER_LOG_ERROR("Master REJECTED authentication");
    }

    return false;
}

bool MasterBroadway::Connect(const string& host, uint16 port, uint8 retryCount,
                             const volatile sig_atomic_t* shutdownFlag)
{
    return ServerBroadwayBase::Connect(host, port, retryCount, &m_pNetClient, shutdownFlag);
}

void MasterBroadway::SendServerKillPacket()
{
    if (!m_pNetClient)
        return;

    VariantVector data(1);
    data[0] = TCP_PACKET_KILL_SERVER;

    m_pNetClient->Send(data);
}

MasterBroadway* GetMasterBroadway()
{
    return MasterBroadway::GetInstance();
}