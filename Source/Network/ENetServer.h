#pragma once

#include "../Precompiled.h"

#include <enet/enet.h>
#include <concurrentqueue.h>

class ENetServer {
public:
    typedef moodycamel::ConcurrentQueue<ENetEvent> EventQueue;

public:
    ENetServer();
    ~ENetServer();

public:
    bool Init(const string& host, uint16 port, uint32 maxPeer = ENET_PROTOCOL_MAXIMUM_PEER_ID);
    void Kill();
    void Update();

    EventQueue& GetEvents() { return m_eventQueue; }

private:
    ENetHost* m_pHost;
    EventQueue m_eventQueue;
};