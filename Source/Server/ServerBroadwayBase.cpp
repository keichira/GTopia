#include "ServerBroadwayBase.h"
#include "../Utils/Timer.h"
#include "IO/Log.h"

ServerBroadwayBase::ServerBroadwayBase() {}

ServerBroadwayBase::~ServerBroadwayBase()
{
    Kill();
}

bool ServerBroadwayBase::Init(const string& host, uint16 port, int32 backLog)
{
    SAFE_DELETE(m_pNetSocket);
    m_pNetSocket = new NetSocket();

    if (!m_pNetSocket->Init(host, port, backLog))
        return false;

    RegisterEvents();
    return true;
}

void ServerBroadwayBase::Kill()
{
    SAFE_DELETE(m_pNetSocket);
}

void ServerBroadwayBase::OnClientConnect(NetClient* pClient) {}

void ServerBroadwayBase::OnClientReceive(NetClient* pClient)
{
    if (!pClient || pClient->status == SOCKET_CLIENT_CLOSE)
        return;

    TCPPacketEvent packet;
    packet.pClient = pClient;
    packet.reqTime = Time::GetSystemTime();

    bool hasPacket = false;

    {
        std::lock_guard<std::mutex> lock(pClient->recvMutex);

        if (pClient->recvQueue.GetDataSize() < sizeof(TCPPacketHeader))
            return;

        pClient->recvQueue.Peek(&packet.header, sizeof(TCPPacketHeader));

        if (packet.header.bodySize > 1024 * 64)
        {
            pClient->status = SOCKET_CLIENT_CLOSE;
            return;
        }

        uint32 totalPacketSize = sizeof(TCPPacketHeader) + packet.header.bodySize;
        if (pClient->recvQueue.GetDataSize() < totalPacketSize)
            return;

        TCPPacketHeader dummyHeader;
        pClient->recvQueue.Read(&dummyHeader, sizeof(TCPPacketHeader));

        if (packet.header.bodySize > 0)
        {
            packet.payload.resize(packet.header.bodySize);
            pClient->recvQueue.Read(packet.payload.data(), packet.header.bodySize);
        }

        hasPacket = true;
    }

    if (!hasPacket)
        return;

    m_packetQueue.enqueue(std::move(packet));

    OnClientReceive(pClient);
}

void ServerBroadwayBase::OnClientDisconnect(NetClient* pClient) {}

void ServerBroadwayBase::RegisterEvents()
{
    m_pNetSocket->GetEvents().Register(
        SOCKET_EVENT_TYPE_CONNECT,
        Delegate<NetClient*>::Create<ServerBroadwayBase, &ServerBroadwayBase::OnClientConnect>(this));

    m_pNetSocket->GetEvents().Register(
        SOCKET_EVENT_TYPE_RECEIVE,
        Delegate<NetClient*>::Create<ServerBroadwayBase, &ServerBroadwayBase::OnClientReceive>(this));

    m_pNetSocket->GetEvents().Register(
        SOCKET_EVENT_TYPE_DISCONNECT,
        Delegate<NetClient*>::Create<ServerBroadwayBase, &ServerBroadwayBase::OnClientDisconnect>(this));
}

void ServerBroadwayBase::Update(bool asClient)
{
    if (!m_pNetSocket)
    {
        return;
    }

    m_pNetSocket->Update(asClient);
}

void ServerBroadwayBase::UpdateTCPLogic(uint64 maxTimeMS) {}

bool ServerBroadwayBase::Connect(const string& host, uint16 port, uint8 retryCount, NetClient** pClient,
                                 const volatile sig_atomic_t* shutdownFlag)
{
    if (!m_pNetSocket)
    {
        return false;
    }

    uint64 connStartTime = Time::GetSystemTime();
    uint8 retriedSoFar = 0;

    m_pNetSocket->Connect(host, port, true);

    while (!*pClient && (!shutdownFlag || *shutdownFlag == 0))
    {
        if (m_pNetSocket)
        {
            m_pNetSocket->Update(true);
        }

        if (retriedSoFar == retryCount)
        {
            break;
        }

        uint64 timeNow = Time::GetSystemTime();
        if (timeNow - connStartTime >= CONNECT_SOCKET_TIMEOUT_MS)
        {
            LOGGER_LOG_ERROR("Failed to connect to socket %s:%d retrying... (%d/%d)", host.c_str(), port,
                             retriedSoFar + 1, retryCount);
            m_pNetSocket->CloseAllClients();
            m_pNetSocket->Connect(host, port, true);
            retriedSoFar++;
            connStartTime = timeNow;
        }

        SleepMS(10);
    }

    return (*pClient != nullptr);
}

bool ServerBroadwayBase::SendPacketRaw(NetClient* pClient, VariantVector& data)
{
    if (!m_pNetSocket)
    {
        return false;
    }

    if (!pClient)
    {
        return false;
    }

    return pClient->Send(data);
}
