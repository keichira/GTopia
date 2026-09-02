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
        if (!event.pClient || event.pClient->status == SOCKET_CLIENT_CLOSE)
            continue;

        uint16 packetID = event.header.packetID;

        if (packetID != TCP_PACKET_HEARTBEAT)
        {
            LOGGER_LOG_DEBUG("Received TCP Packet %d (Size: %u)", packetID, event.header.bodySize);
        }

        TCPPacketReader reader(event.payload.data(), (uint32)(event.payload.size()));

        if (packetID == TCP_PACKET_PLAYER_SUBSCRIBE || packetID == TCP_PACKET_PLAYER_UNSUBSCRIBE ||
            packetID == TCP_PACKET_WORLD_SNAPSHOT || packetID == TCP_PACKET_WORLD_UPDATE ||
            packetID == TCP_PACKET_WORLD_REMOVE)
        {
            GetGamePresenceManager()->OnTCPPacket(packetID, reader);
        }
        else
        {
            m_events.Dispatch(packetID, event.pClient, event.header, reader);
        }

        if (startTime.GetElapsedTime() >= maxTimeMS)
            break;
    }
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

    TCPPacketWriter writer(TCP_PACKET_PLAYER_CHECK_SESSION);
    writer.Write(netID);
    writer.Write(userID);
    writer.Write(token);
    writer.Write(serverID);

    m_pNetClient->Send(writer);
}

void MasterBroadway::SendRenderWorldRequest(uint32 userID, uint32 worldInstanceID)
{
    if (!m_pNetClient)
        return;

    TCPPacketWriter writer(TCP_PACKET_RENDER_WORLD);
    writer.Write<int32>(TCP_RENDER_REQUEST);
    writer.Write(userID);
    writer.Write(worldInstanceID);

    m_pNetClient->Send(writer);
}

void MasterBroadway::SendWorldInitResult(bool succeed, uint32 worldInstanceID)
{
    if (!m_pNetClient)
        return;

    TCPPacketWriter writer(TCP_PACKET_WORLD_INIT);
    writer.Write<int32>(succeed ? TCP_RESULT_OK : TCP_RESULT_FAIL);
    writer.Write(worldInstanceID);

    m_pNetClient->Send(writer);
}

void MasterBroadway::SendPlayerWorldJoin(uint32 playerUserID, const string& worldName, const string& doorID)
{
    if (!m_pNetClient)
        return;

    TCPPacketWriter writer(TCP_PACKET_WORLD_SEND_PLAYER);
    writer.Write(playerUserID);
    writer.WriteString(worldName);
    writer.WriteString(doorID);

    m_pNetClient->Send(writer);
}

void MasterBroadway::SendHeartBeat()
{
    if (!m_pNetClient)
        return;

    TCPPacketWriter writer(TCP_PACKET_HEARTBEAT);
    writer.Write<uint32>(GetPlayerManager()->GetPlayerCount());
    writer.Write<uint32>(GetWorldManager()->GetWorldCount());

    m_pNetClient->Send(writer);
    m_lastHearthBeatSentTime.Reset();
}

void MasterBroadway::SendEndPlayerSession(uint32 userID)
{
    if (!m_pNetClient)
        return;

    TCPPacketWriter writer(TCP_PACKET_PLAYER_END_SESSION);
    writer.Write(userID);

    m_pNetClient->Send(writer);
}

void MasterBroadway::SendPlayerJoinedWorld(uint32 playerUserID, uint32 worldInstanceID)
{
    if (!m_pNetClient)
        return;

    TCPPacketWriter writer(TCP_PACKET_WORLD_PLAYER_SESSION);
    writer.Write<int32>(TCP_WORLD_PLAYER_JOIN);
    writer.Write(playerUserID);
    writer.Write(worldInstanceID);

    m_pNetClient->Send(writer);
}

void MasterBroadway::SendPlayerLeftWorld(uint32 playerUserID, uint32 worldInstanceID)
{
    if (!m_pNetClient)
        return;

    TCPPacketWriter writer(TCP_PACKET_WORLD_PLAYER_SESSION);
    writer.Write<int32>(TCP_WORLD_PLAYER_LEAVE);
    writer.Write(playerUserID);
    writer.Write(worldInstanceID);

    m_pNetClient->Send(writer);
}

void MasterBroadway::SendHelloPacket()
{
    if (!m_pNetClient)
        return;

    TCPPacketWriter writer(TCP_PACKET_HELLO);
    m_pNetClient->Send(writer);
}

void MasterBroadway::SendAuthPacket(const string& authKey)
{
    if (!m_pNetClient)
        return;

    TCPPacketWriter writer(TCP_PACKET_AUTH);
    writer.WriteString(authKey);
    writer.Write<uint32>(GetContext()->GetID());
    writer.Write<int32>(CONFIG_SERVER_GAME);

    m_pNetClient->Send(writer);
}

void MasterBroadway::SendServerKillPacket()
{
    if (!m_pNetClient)
        return;

    TCPPacketWriter writer(TCP_PACKET_KILL_SERVER);
    m_pNetClient->Send(writer);
}

void MasterBroadway::SendPlayerPresenceSubscribe(const std::vector<uint32>& userIDs)
{
    SendArray(TCP_PACKET_PLAYER_SUBSCRIBE, userIDs);
}

void MasterBroadway::SendPlayerPresenceUnsubscribe(const std::vector<uint32>& userIDs)
{
    SendArray(TCP_PACKET_PLAYER_UNSUBSCRIBE, userIDs);
}

void MasterBroadway::SendWorldPresenceRemove(const std::vector<WorldPresenceRemoveElement>& removeElems)
{
    SendArray(TCP_PACKET_WORLD_REMOVE, removeElems);
}

void MasterBroadway::SendWorldPresenceUpdate(const std::vector<WorldPresenceUpdateElement>& updateElems)
{
    SendArray(TCP_PACKET_WORLD_UPDATE, updateElems);
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

MasterBroadway* GetMasterBroadway()
{
    return MasterBroadway::GetInstance();
}