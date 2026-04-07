#include "ServerBroadwayBase.h"
#include "../Utils/Timer.h"
#include "IO/Log.h"

ServerBroadwayBase::ServerBroadwayBase()
: m_connected(false), m_pNetClient(nullptr)
{
}

ServerBroadwayBase::~ServerBroadwayBase()
{
    Kill();
}

bool ServerBroadwayBase::Init(const string& host, uint16 port, int32 backLog)
{
    m_pNetSocket = new NetSocket();
    if(!m_pNetSocket->Init(host, port, backLog)) {
        return false;
    }

    RegisterEvents();
    return true;
}

void ServerBroadwayBase::Kill()
{
    m_pNetClient = nullptr;
    m_connected = false;
    
    SAFE_DELETE(m_pNetSocket);
}

void ServerBroadwayBase::OnClientConnect(NetClient* pClient)
{
    if(!pClient || pClient->status == SOCKET_CLIENT_CLOSE) {
        return;
    }

    m_pNetClient = pClient;
    m_connected = true;
}

void ServerBroadwayBase::OnClientReceive(NetClient* pClient)
{
    if(!pClient || pClient->status == SOCKET_CLIENT_CLOSE) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(pClient->mutex);
        if(pClient->recvQueue.GetDataSize() <= sizeof(uint32)) {
            return;
        }

        uint32 packetSize = 0;
        pClient->recvQueue.Peek(&packetSize, sizeof(uint32));

        if(pClient->recvQueue.GetDataSize() < packetSize + sizeof(uint32)) {
            return;
        }
        else if(packetSize >= 1024 * 2) {
            pClient->status = SOCKET_CLIENT_CLOSE;
            return;
        }
    }

    VariantVector data;
    {
        std::lock_guard<std::mutex> lock(pClient->mutex);
        
        uint32 packetSize = 0;
        pClient->recvQueue.Read(&packetSize, sizeof(uint32));

        uint8* pPacketData = new uint8[packetSize];
        pClient->recvQueue.Read(pPacketData, packetSize);

        MemoryBuffer memBuffer(pPacketData, packetSize);

        DeSerializeVariantVectorForTCP(memBuffer, data);
        SAFE_DELETE_ARRAY(pPacketData);
    }

    TCPPacketEvent packet;
    packet.data = std::move(data);
    packet.pClient = pClient;
    packet.reqTime = Time::GetSystemTime();

    m_packetQueue.enqueue(std::move(packet));
}

void ServerBroadwayBase::OnClientDisconnect(NetClient* pClient)
{
}

void ServerBroadwayBase::RegisterEvents()
{
    m_pNetSocket->GetEvents().Register(
        SOCKET_EVENT_TYPE_CONNECT, 
        Delegate<NetClient*>::Create<ServerBroadwayBase, &ServerBroadwayBase::OnClientConnect>(this)
    );

    m_pNetSocket->GetEvents().Register(
        SOCKET_EVENT_TYPE_RECEIVE, 
        Delegate<NetClient*>::Create<ServerBroadwayBase, &ServerBroadwayBase::OnClientReceive>(this)
    );

    m_pNetSocket->GetEvents().Register(
        SOCKET_EVENT_TYPE_DISCONNECT, 
        Delegate<NetClient*>::Create<ServerBroadwayBase, &ServerBroadwayBase::OnClientDisconnect>(this)
    );
}

void ServerBroadwayBase::Update(bool asClient)
{
    if(!m_pNetSocket) {
        return;
    }
    
    m_pNetSocket->Update(asClient);
}

void ServerBroadwayBase::UpdateTCPLogic(uint64 maxTimeMS)
{
}

bool ServerBroadwayBase::Connect(const string& host, uint16 port, uint8 retryCount, const volatile sig_atomic_t* shutdownFlag)
{
    if(!m_pNetSocket) {
        return false;
    }
    
    uint64 connStartTime = Time::GetSystemTime();
    uint8 retriedSoFar = 0;

    m_pNetSocket->Connect(host, port, true);

    while(!m_connected && (!shutdownFlag || *shutdownFlag == 0)) {
        m_pNetSocket->Update(true);

        if(retriedSoFar == retryCount) {
            break;
        }

        uint64 timeNow = Time::GetSystemTime();
        if(timeNow - connStartTime >= CONNECT_SOCKET_TIMEOUT_MS) {
            LOGGER_LOG_ERROR("Failed to connect to socket %s:%d retrying... (%d/%d)", host.c_str(), port, retriedSoFar + 1, retryCount);
            m_pNetSocket->CloseAllClients();
            m_pNetSocket->Connect(host, port, true);
            retriedSoFar++;
            connStartTime = timeNow;
        }

        SleepMS(10);
    }

    return m_connected;
}

bool ServerBroadwayBase::SendPacketRaw(VariantVector& data)
{
    if(!m_pNetSocket) {
        return false;
    }

    if(!m_pNetClient) {
        return false;
    }

    return m_pNetClient->Send(data);
}
