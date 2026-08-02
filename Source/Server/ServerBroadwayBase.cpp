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
    {
        return false;
    }

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

    uint32 packetSize = 0;
    uint8* pPacketData = nullptr;
    bool hasFullPacket = false;

    {
        std::lock_guard<std::mutex> lock(pClient->recvMutex);

        if (pClient->recvQueue.GetDataSize() <= sizeof(uint32))
            return;

        pClient->recvQueue.Peek(&packetSize, sizeof(uint32));

        if (pClient->recvQueue.GetDataSize() < packetSize + sizeof(uint32))
            return;

        if (packetSize >= 1024 * 6 || packetSize == 0)
        {
            pClient->status = SOCKET_CLIENT_CLOSE;
            return;
        }

        uint32 dummySize = 0;
        pClient->recvQueue.Read(&dummySize, sizeof(uint32));

        pPacketData = new uint8[packetSize];
        pClient->recvQueue.Read(pPacketData, packetSize);
        hasFullPacket = true;
    }

    if (!hasFullPacket)
        return;

    if (packetSize >= 3 && pPacketData[0] == 0xFF)
    {
        packet.isRaw = true;
        packet.packetType = *(uint16*)(pPacketData + 1);

        uint32 rawPayloadSize = packetSize - 3;
        if (rawPayloadSize > 0)
        {
            packet.rawData.assign(pPacketData + 3, pPacketData + packetSize);
        }
    }
    else
    {
        packet.isRaw = false;
        MemoryBuffer memBuffer(pPacketData, packetSize);
        DeSerializeVariantVectorForTCP(memBuffer, packet.data);

        if (!packet.data.empty() && packet.data[0].GetType() == VARIANT_TYPE_INT)
            packet.packetType = (uint16)packet.data[0].GetINT();
        else
            packet.packetType = 0;
    }

    SAFE_DELETE_ARRAY(pPacketData);
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
