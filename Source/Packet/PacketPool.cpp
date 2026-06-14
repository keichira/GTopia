#include "PacketPool.h"

PacketPool gPacketPool;

PacketPool::PacketPool()
{
}

PacketPool::~PacketPool()
{
}

void PacketPool::Init(PacketPoolConfig& config)
{
    m_config = config;
    uint32 totalNodes = m_config.smallPoolSize + m_config.medPoolSize + m_config.largePoolSize + m_config.hugePoolSize;
    m_packetNodes.resize(totalNodes);

    m_smallBuffer.resize(m_config.smallPoolSize * m_config.smallPacketSize);
    m_medBuffer.resize(m_config.medPoolSize * m_config.medPacketSize);
    m_largeBuffer.resize(m_config.largePoolSize * m_config.largePacketSize);
    m_hugeBuffer.resize(m_config.hugePoolSize * m_config.hugePacketSize);

    uint32 nodeIdx = 0;

    for(uint32 i = 0; i < m_config.smallPoolSize; ++i) {
        m_packetNodes[nodeIdx].payload = &m_smallBuffer[i * m_config.smallPacketSize];
        m_packetNodes[nodeIdx].maxBufferSize = m_config.smallPacketSize;
        m_smallPool.enqueue(&m_packetNodes[nodeIdx++]);
    }
    for(uint32 i = 0; i < m_config.medPoolSize; ++i) {
        m_packetNodes[nodeIdx].payload = &m_medBuffer[i * m_config.medPacketSize];
        m_packetNodes[nodeIdx].maxBufferSize = m_config.medPacketSize;
        m_medPool.enqueue(&m_packetNodes[nodeIdx++]);
    }
    for(uint32 i = 0; i < m_config.largePoolSize; ++i) {
        m_packetNodes[nodeIdx].payload = &m_largeBuffer[i * m_config.largePacketSize];
        m_packetNodes[nodeIdx].maxBufferSize = m_config.largePacketSize;
        m_largePool.enqueue(&m_packetNodes[nodeIdx++]);
    }
    for(uint32 i = 0; i < m_config.hugePoolSize; ++i) {
        m_packetNodes[nodeIdx].payload = &m_hugeBuffer[i * m_config.hugePacketSize];
        m_packetNodes[nodeIdx].maxBufferSize = m_config.hugePacketSize;
        m_hugePool.enqueue(&m_packetNodes[nodeIdx++]);
    }
}

PooledPacket* PacketPool::Acquire(uint32 size)
{
    PooledPacket* pPacket = nullptr;

    if(size <= m_config.smallPacketSize && m_smallPool.try_dequeue(pPacket)) 
        return pPacket;
    
    if(size <= m_config.medPacketSize && m_medPool.try_dequeue(pPacket)) 
        return pPacket;
    
    if(size <= m_config.largePacketSize && m_largePool.try_dequeue(pPacket)) 
        return pPacket;
    
    if(size <= m_config.hugePacketSize && m_hugePool.try_dequeue(pPacket)) 
        return pPacket;

    return pPacket;
}

void PacketPool::Release(PooledPacket* pPacket)
{
    if(!pPacket)
        return;

    uint32 originalSize = pPacket->maxBufferSize;
    pPacket->dataLength = 0;

    if (originalSize == m_config.smallPacketSize) m_smallPool.enqueue(pPacket);
    else if (originalSize == m_config.medPacketSize) m_medPool.enqueue(pPacket);
    else if (originalSize == m_config.largePacketSize) m_largePool.enqueue(pPacket);
    else if (originalSize == m_config.hugePacketSize) m_hugePool.enqueue(pPacket);
}

bool PacketPool::IsHugeSlotAvailable()
{
    return m_hugePool.size_approx() > 0;   
}