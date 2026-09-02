#include "MasterBroadway.h"
#include "Context.h"
#include "IO/Log.h"
#include "Utils/Timer.h"

#include "Event/TCPEvent_Server.h"
#include "Event/TCPEvent_World.h"

MasterBroadway::MasterBroadway() {}

MasterBroadway::~MasterBroadway() {}

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

void MasterBroadway::RegisterEvents()
{
    ServerBroadwayBase::RegisterEvents();

    RegisterEvent<TCPEvent_Hello>(TCP_PACKET_HELLO);
    RegisterEvent<TCPEvent_Auth>(TCP_PACKET_AUTH);
    RegisterEvent<TCPEvent_RenderWorld>(TCP_PACKET_RENDER_WORLD);
}

void MasterBroadway::UpdateTCPLogic(uint64 maxTimeMS)
{
    Timer startTime;
    TCPPacketEvent event;

    uint32 processed = 0;

    while (m_packetQueue.try_dequeue(event))
    {
        if (!event.pClient)
            continue;

        TCPPacketReader reader(event.payload.data(), (uint32)(event.payload.size()));
        m_events.Dispatch(event.header.packetID, event.pClient, event.header, reader);

        if (startTime.GetElapsedTime() >= maxTimeMS)
            break;
    }
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

    /**
     * for now just send back the string
     * actually NetSocket was supporting TLS but removed it for now
     * planned to use HMAC for here for non-TLS socket but openssl lib is so big
     */

    TCPPacketWriter writer(TCP_PACKET_AUTH);
    writer.WriteString(authKey);
    writer.Write<uint32>(GetContext()->GetID());
    writer.Write<uint8>(CONFIG_SERVER_RENDERER);

    m_pNetClient->Send(writer);
}

/**
 * set flag to world if player trying to change world name with address
 * and block it
 */
void MasterBroadway::SendWorldRenderResult(bool succeed, uint32 userID, uint32 worldID)
{
    if (!m_pNetClient)
        return;

    TCPPacketWriter writer(TCP_PACKET_RENDER_WORLD);
    writer.Write<int32>(TCP_RENDER_RESULT);
    writer.Write<uint8>(succeed ? TCP_RESULT_OK : TCP_RESULT_FAIL);
    writer.Write(userID);
    writer.Write(worldID);

    m_pNetClient->Send(writer);
}

void MasterBroadway::SendServerKillPacket()
{
    if (!m_pNetClient)
        return;

    TCPPacketWriter writer(TCP_PACKET_KILL_SERVER);
    m_pNetClient->Send(writer);
}

bool MasterBroadway::Connect(const string& host, uint16 port, uint8 retryCount,
                             const volatile sig_atomic_t* shutdownFlag)
{
    return ServerBroadwayBase::Connect(host, port, retryCount, &m_pNetClient, shutdownFlag);
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

MasterBroadway* GetMasterBroadway()
{
    return MasterBroadway::GetInstance();
}