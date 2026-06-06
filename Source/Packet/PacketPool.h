#pragma once

#include "../Precompiled.h"
#include <concurrentqueue.h>

struct PooledPacket
{
    uint32 dataLength = 0;
    uint8* payload;

    uint32 maxBufferSize = 0;
    bool isDynamic = false;
};

struct PacketPoolConfig
{
    uint32 loginPoolSize = 0;
    uint32 loginPacketSize = 0;

    uint32 textPoolSize = 0;
    uint32 textPacketSize = 0;

    uint32 gamePoolSize = 0;
    uint32 gamePacketSize = 0;
};

class PacketPool {
public:
    PacketPool();
    ~PacketPool();

public:
    void Init(PacketPoolConfig& config);

    PooledPacket* Acquire(uint32 size, bool allowDynamic);
    void Release(PooledPacket* pPacket);

private:
    PacketPoolConfig m_config;
    std::vector<PooledPacket> m_packetNodes;

    std::vector<uint8> m_loginBuffer;
    std::vector<uint8> m_textBuffer;
    std::vector<uint8> m_gameBuffer;

    moodycamel::ConcurrentQueue<PooledPacket*> m_loginPool;
    moodycamel::ConcurrentQueue<PooledPacket*> m_textPool;
    moodycamel::ConcurrentQueue<PooledPacket*> m_gamePool;
};

extern PacketPool gPacketPool;