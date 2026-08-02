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
        {
            continue;
        }

        int8 type = event.data[0].GetINT();
        m_events.Dispatch(type, event.pClient, event.data);

        if (startTime.GetElapsedTime() >= maxTimeMS)
        {
            break;
        }
    }
}

void MasterBroadway::SendHelloPacket()
{
    if (!m_pNetClient)
    {
        return;
    }

    VariantVector data(1);
    data[0] = TCP_PACKET_HELLO;

    m_pNetClient->Send(data);
}

void MasterBroadway::SendAuthPacket(const string& authKey)
{
    if (!m_pNetClient)
    {
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
    packet[3] = CONFIG_SERVER_RENDERER;

    m_pNetClient->Send(packet);
}

/**
 * set flag to world if player trying to change world name with address
 * and block it
 */
void MasterBroadway::SendWorldRenderResult(bool succeed, uint32 userID, uint32 worldID)
{
    VariantVector data(5);
    data[0] = TCP_PACKET_RENDER_WORLD;
    data[1] = TCP_RENDER_RESULT;
    data[2] = succeed ? TCP_RESULT_OK : TCP_RESULT_FAIL;
    data[3] = userID;
    data[4] = worldID;

    m_pNetClient->Send(data);
}

void MasterBroadway::SendServerKillPacket()
{
    if (!m_pNetClient)
    {
        return;
    }

    VariantVector data(2);
    data[0] = TCP_PACKET_KILL_SERVER;

    m_pNetClient->Send(data);
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