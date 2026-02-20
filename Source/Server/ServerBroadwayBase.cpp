#include "ServerBroadwayBase.h"
#include "../Utils/Timer.h"

#include "IO/Log.h"

ServerBroadwayBase::ServerBroadwayBase()
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
    SAFE_DELETE(m_pNetSocket);
}

void ServerBroadwayBase::OnClientConnect(NetClient* pClient)
{
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
    m_pNetSocket->GetEvents().Register(SOCKET_EVENT_TYPE_CONNECT, *this, &ServerBroadwayBase::OnClientConnect);
    m_pNetSocket->GetEvents().Register(SOCKET_EVENT_TYPE_RECEIVE, *this, &ServerBroadwayBase::OnClientReceive);
    m_pNetSocket->GetEvents().Register(SOCKET_EVENT_TYPE_DISCONNECT, *this, &ServerBroadwayBase::OnClientDisconnect);
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
