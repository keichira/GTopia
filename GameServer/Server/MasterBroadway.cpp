#include "MasterBroadway.h"
#include "IO/Log.h"
#include "Packet/NetPacket.h"
#include "Utils/Timer.h"

#include "../Event/TCP/TCPEventHello.h"
#include "../Event/TCP/TCPEventAuth.h"
#include "../Event/TCP/TCPEventPlayerSession.h"
#include "../Event/TCP/TCPEventWorldInit.h"

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

    m_pNetClient = pClient;
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

    m_events.Register(
        TCP_PACKET_PLAYER_CHECK_SESSION,
        Delegate<NetClient*, VariantVector&>::Create<&TCPEventPlayerSession::Execute>()
    );

    m_events.Register(
        TCP_PACKET_WORLD_INIT,
        Delegate<NetClient*, VariantVector&>::Create<&TCPEventWorldInit::Execute>()
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
    if(!m_connected || !m_pNetClient) {
        return;
    }

    VariantVector data(1);
    data[0] = TCP_PACKET_HELLO;

    m_pNetClient->Send(data);
}

void MasterBroadway::SendCheckSessionPacket(int32 netID, uint32 userID, uint32 token, uint16 serverID)
{
    if(!m_connected || !m_pNetClient) {
        return;
    }

    VariantVector data(5);
    data[0] = TCP_PACKET_PLAYER_CHECK_SESSION;
    data[1] = netID;
    data[2] = userID;
    data[3] = token;
    data[4] = serverID;

    m_pNetClient->Send(data);
}

bool MasterBroadway::SendPacketRaw(VariantVector& data)
{
    if(!m_pNetSocket) {
        return false;
    }

    if(!m_pNetClient) {
        return false;
    }

    return m_pNetClient->Send(data);
}

MasterBroadway* GetMasterBroadway() { return MasterBroadway::GetInstance(); }