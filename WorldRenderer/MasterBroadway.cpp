#include "MasterBroadway.h"
#include "IO/Log.h"
#include "Utils/Timer.h"
#include "Context.h"

#include "Event/TCPEventHello.h"
#include "Event/TCPEventAuth.h"
#include "Event/TCPEventRenderWorld.h"

MasterBroadway::MasterBroadway()
{
}

MasterBroadway::~MasterBroadway()
{
}

void MasterBroadway::OnClientConnect(NetClient* pClient)
{
    if(m_pNetClient && pClient) {
        pClient->status = SOCKET_CLIENT_CLOSE;
    }
    else if(!m_pNetClient && pClient) {
        m_pNetClient = pClient;
    }
}

void MasterBroadway::OnClientDisconnect(NetClient* pClient)
{
    if(pClient && m_pNetClient && (m_pNetClient == pClient)) {
        m_pNetClient = nullptr;
    }
}

void MasterBroadway::RegisterEvents()
{
    ServerBroadwayBase::RegisterEvents();

    RegisterEvent<TCPEventHello>(TCP_PACKET_HELLO);
    RegisterEvent<TCPEventAuth>(TCP_PACKET_AUTH);
    RegisterEvent<TCPEventRenderWorld>(TCP_PACKET_RENDER_WORLD);
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

void MasterBroadway::SendHelloPacket()
{
    if(!m_pNetClient) {
        return;
    }

    VariantVector data(1);
    data[0] = TCP_PACKET_HELLO;

    m_pNetClient->Send(data);
}

void MasterBroadway::SendAuthPacket(const string& authKey)
{
    if(!m_pNetClient) {
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
    if(!m_pNetClient) {
        return;
    }

    VariantVector data(2);
    data[0] = TCP_PACKET_KILL_SERVER;
    data[1] = (uint32)GetContext()->GetID();

    m_pNetClient->Send(data);   
}

bool MasterBroadway::Connect(const string& host, uint16 port, uint8 retryCount, const volatile sig_atomic_t* shutdownFlag)
{
    return ServerBroadwayBase::Connect(host, port, retryCount, &m_pNetClient, shutdownFlag);
}

MasterBroadway* GetMasterBroadway() { return MasterBroadway::GetInstance(); }