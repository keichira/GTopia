#include "MasterBroadway.h"
#include "IO/Log.h"
#include "Packet/NetPacket.h"
#include "Utils/Timer.h"

#include "../Event/TCP/TCPEventHello.h"
#include "../Event/TCP/TCPEventAuth.h"

MasterBroadway::MasterBroadway()
: m_connected(false)
{
}

MasterBroadway::~MasterBroadway()
{
}

void MasterBroadway::OnClientConnect(NetClient* pClient)
{
    if(!pClient) {
        return;
    }

    m_pClient = pClient;
    m_connected = true;
}

void MasterBroadway::RegisterEvents()
{
    ServerBroadwayBase::RegisterEvents();

    m_events.Register(
        TCP_PACKET_HELLO,
        Delegate<NetClient*, VariantVector&>::Create<&TCPEventHello::Execute>()
    );

    m_events.Register(
        TCP_PACKET_AUTH,
        Delegate<NetClient*, VariantVector&>::Create<&TCPEventAuth::Execute>()
    );
}

void MasterBroadway::UpdateTCPLogic(uint64 maxTimeMS)
{
    uint64 startTime = Time::GetSystemTime();
    TCPPacketEvent event;

    uint32 processed = 0;

    while(m_packetQueue.try_dequeue(event)) {
        if(!event.pClient) {
            continue;
        }

        int8 type = event.data[0].GetINT();

        m_events.Dispatch(type, event.pClient, event.data);

        processed++;
        if(Time::GetSystemTime() - startTime >= maxTimeMS) {
            break;
        }
    }

    if(processed > 0) {
        LOGGER_LOG_DEBUG("Processed %d TCP packets maxMS %d, took %d MS", processed, maxTimeMS, Time::GetSystemTime() - startTime);
    }
}

void MasterBroadway::Connect(const string& host, uint16 port)
{
    if(!m_pNetSocket) {
        return;
    }

    m_pNetSocket->Connect(host, port, true);
}

void MasterBroadway::RefreshForConnect()
{
    m_pNetSocket->CloseAllClients();
}

void MasterBroadway::SendHelloPacket()
{
    if(!m_connected || !m_pClient) {
        return;
    }

    VariantVector data(1);
    data[0] = TCP_PACKET_HELLO;

    m_pClient->Send(data);
}

void MasterBroadway::SendCheckSessionPacket(int32 netID, uint32 userID, uint32 token, uint16 serverID)
{
    if(!m_connected || !m_pClient) {
        return;
    }

    VariantVector data(5);
    data[0] = TCP_PACKET_PLAYER_SESSION;
    data[1] = netID;
    data[2] = userID;
    data[3] = token;
    data[4] = serverID;

    m_pClient->Send(data);
}

MasterBroadway* GetMasterBroadway() { return MasterBroadway::GetInstance(); }