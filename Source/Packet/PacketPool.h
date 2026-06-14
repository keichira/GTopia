#pragma once

#include "../Precompiled.h"
#include <concurrentqueue.h>

struct PooledPacket
{
    uint32 dataLength = 0;
    uint8* payload;
    uint32 maxBufferSize = 0;
};

struct PacketPoolConfig
{
    uint32 smallPoolSize = 0;
    uint32 smallPacketSize = 0;

    uint32 medPoolSize = 0;
    uint32 medPacketSize = 0;

    uint32 largePoolSize = 0;
    uint32 largePacketSize = 0;

    uint32 hugePoolSize = 0;
    uint32 hugePacketSize = 0;
};

class PacketPool {
public:
    PacketPool();
    ~PacketPool();

public:
    void Init(PacketPoolConfig& config);

    PooledPacket* Acquire(uint32 size);
    void Release(PooledPacket* pPacket);
    bool IsHugeSlotAvailable();

private:
    PacketPoolConfig m_config;
    std::vector<PooledPacket> m_packetNodes;

    std::vector<uint8> m_smallBuffer;
    std::vector<uint8> m_medBuffer;
    std::vector<uint8> m_largeBuffer;
    std::vector<uint8> m_hugeBuffer;

    moodycamel::ConcurrentQueue<PooledPacket*> m_smallPool;
    moodycamel::ConcurrentQueue<PooledPacket*> m_medPool;
    moodycamel::ConcurrentQueue<PooledPacket*> m_largePool;
    moodycamel::ConcurrentQueue<PooledPacket*> m_hugePool;
};

extern PacketPool gPacketPool;