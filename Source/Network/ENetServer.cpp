#include "ENetServer.h"

ENetServer::ENetServer() : m_pHost(nullptr) {}

ENetServer::~ENetServer()
{
    Kill();
}

bool ENetServer::Init(const string& host, uint16 port, uint32 maxPeer)
{
    if (enet_initialize() != 0)
    {
        return false;
    }

    ENetAddress addr{};
    if (!host.empty())
    {
        if (enet_address_set_host(&addr, host.c_str()) != 0)
        {
            return false;
        }
    }
    else
    {
        addr.host = ENET_HOST_ANY;
    }
    addr.port = port;

    m_pHost = enet_host_create(&addr, maxPeer, 2, 0, 0);
    if (!m_pHost)
    {
        return false;
    }

    m_pHost->checksum = enet_crc32;
    enet_host_compress_with_range_coder(m_pHost);

    return true;
}

void ENetServer::Kill()
{
    if (m_pHost)
    {
        enet_host_flush(m_pHost);
        enet_host_destroy(m_pHost);
        m_pHost = nullptr;
    }

    enet_deinitialize();
}

bool ENetServer::Update(ENetEvent* pEvent)
{
    if (!m_pHost || !pEvent)
        return false;

    return enet_host_check_events(m_pHost, pEvent) > 0;
}

void ENetServer::Service()
{
    if (!m_pHost)
        return;

    enet_host_service(m_pHost, nullptr, 0);
}

void ENetServer::Wait(uint32 timeoutMS)
{
    if (!m_pHost)
        return;

    enet_uint32 condition = ENET_SOCKET_WAIT_RECEIVE;
    enet_socket_wait(m_pHost->socket, &condition, timeoutMS);
}

void ENetServer::Flush()
{
    if (m_pHost)
    {
        enet_host_flush(m_pHost);
    }
}

void ENetServer::SetENetIncomeCmdType(ENetHostIncomeCommandType type)
{
    if (!m_pHost)
        return;

    m_pHost->incomeCommandType = type;
}
