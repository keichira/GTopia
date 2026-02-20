#include "MasterBroadway.h"
#include "IO/Log.h"
#include "Packet/NetPacket.h"

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

void MasterBroadway::Connect(const string& host, uint16 port)
{
    if(!m_pNetSocket) {
        return;
    }

    m_pNetSocket->Connect(host, port);
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

MasterBroadway* GetMasterBroadway() { return MasterBroadway::GetInstance(); }