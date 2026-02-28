#include "ServerBase.h"
#include "../Utils/Timer.h"
#include "../IO/Log.h"

ServerBase::ServerBase()
{
}

ServerBase::~ServerBase()
{
    Kill();
}

void ServerBase::OnEventConnect(ENetEvent &event)
{
}

void ServerBase::OnEventReceive(ENetEvent &event)
{
}

void ServerBase::OnEventDisconnect(ENetEvent &event)
{
}

void ServerBase::RegisterEvents()
{
}

bool ServerBase::Init(const string &host, uint16 port)
{
    m_pENetServer = new ENetServer();
    if(!m_pENetServer->Init(host, port)) {
        return false;
    }

    RegisterEvents();
    return true;
}

void ServerBase::Kill()
{
    SAFE_DELETE(m_pENetServer);
}

void ServerBase::Update()
{
    if(!m_pENetServer) {
        return;
    }

    m_pENetServer->Update();
}

void ServerBase::UpdateGameLogic(uint64 maxTimeMS)
{
    if(!m_pENetServer) {
        return;
    }

    uint64 startTime = Time::GetSystemTime();
    ENetEvent event;

    uint32 processedPacketCount = 0;

    while(m_pENetServer->GetEvents().try_dequeue(event)) {
        switch(event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                OnEventConnect(event);
                break;
            }

            case ENET_EVENT_TYPE_RECEIVE: {
                OnEventReceive(event);
                enet_packet_destroy(event.packet);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT: {
                OnEventDisconnect(event);
                break;
            }

            default:
                break;
        }

        processedPacketCount++;
        if(Time::GetSystemTime() - startTime >= maxTimeMS) {
            break;
        }
    }

    if(processedPacketCount > 0) {
        LOGGER_LOG_DEBUG("Processed %d ENet packets maxMS %d, took %d MS", processedPacketCount, maxTimeMS, Time::GetSystemTime() - startTime);
    }
}

GamePlayer* ServerBase::GetPlayerByNetID(int32 netID)
{
    auto it = m_playerCache.find(netID);
    if(it != m_playerCache.end()) {
        return it->second;
    }

    return nullptr;
}
