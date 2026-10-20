#include "ServerBase.h"
#include "../IO/Log.h"
#include "../Utils/Timer.h"

ServerBase::ServerBase() : m_lastConnectionID(0) {}

ServerBase::~ServerBase()
{
    Kill();
}

void ServerBase::OnEventConnect(NetworkEvent& event) {}

void ServerBase::OnEventReceive(NetworkEvent& event) {}

void ServerBase::OnEventDisconnect(NetworkEvent& event) {}

void ServerBase::RegisterEvents() {}

bool ServerBase::Init(const string& host, uint16 port)
{
    m_pENetServer = new ENetServer();
    if (!m_pENetServer->Init(host, port))
        return false;

    RegisterEvents();
    return true;
}

void ServerBase::Kill()
{
    SAFE_DELETE(m_pENetServer);
}

void ServerBase::Update() {}

void ServerBase::SetENetIncomeCmdType(uint8 type)
{
    if (type == 1)
        m_pENetServer->SetENetIncomeCmdType(ENET_HOST_INCOME_CMD_DEFAULT);
    else if (type == 2)
        m_pENetServer->SetENetIncomeCmdType(ENET_HOST_INCOME_CMD_GT_PROTOCOL);
    else
        m_pENetServer->SetENetIncomeCmdType(ENET_HOST_INCOME_CMD_BOTH);
}

void ServerBase::WaitForNetwork()
{
    if (!m_pENetServer)
        return;

    if (gPacketOutgoingQueue.size_approx() == 0)
        m_pENetServer->Wait(1);
}

void ServerBase::KickPeer(uint32 netID, ENetPeer* pPeer)
{
    if (pPeer)
    {
        pPeer->data = nullptr;
        enet_peer_disconnect_now(pPeer, 0);
    }

    m_connectionMap.erase(netID);
    NetworkEvent netEvent{ENET_EVENT_TYPE_DISCONNECT, netID, nullptr, nullptr};
    m_networkQueue.enqueue(std::move(netEvent));
}

bool ServerBase::BudgetExpired(uint32 processed, uint64 startUS)
{
    return (processed & 31) == 31 && Time::GetTimeNowUS() - startUS >= 4000;
}

void ServerBase::UpdateGameLogic(uint64 maxTimeMS)
{
    Timer startTime;
    NetworkEvent netEvent;

    while (m_networkQueue.try_dequeue(netEvent))
    {
        switch (netEvent.type)
        {
            case ENET_EVENT_TYPE_CONNECT:
            {
                OnEventConnect(netEvent);
                break;
            }

            case ENET_EVENT_TYPE_RECEIVE:
            {
                OnEventReceive(netEvent);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT:
            {
                OnEventDisconnect(netEvent);
                break;
            }

            default:
                break;
        }

        if (startTime.GetElapsedTime() >= maxTimeMS)
        {
            break;
        }
    }
}
