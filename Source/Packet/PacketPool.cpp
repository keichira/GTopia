#include "PacketPool.h"

PacketPool gPacketPool;

PacketPool::PacketPool()
{
}

PacketPool::~PacketPool()
{
}

void PacketPool::Init(PacketPoolConfig &config)
{
    m_config = config;

    uint32 totalNodes = m_config.loginPoolSize + m_config.textPoolSize + m_config.gamePoolSize;
    m_packetNodes.resize(totalNodes);

    m_loginBuffer.resize(m_config.loginPoolSize * m_config.loginPacketSize);
    m_textBuffer.resize(m_config.textPoolSize * m_config.textPacketSize);
    m_gameBuffer.resize(m_config.gamePoolSize * m_config.gamePacketSize);

    uint32 nodeIdx = 0;

    for(uint32 i = 0; i < m_config.loginPoolSize; ++i) 
    {
        m_packetNodes[nodeIdx].payload = &m_loginBuffer[i * m_config.loginPacketSize];
        m_packetNodes[nodeIdx].maxBufferSize = m_config.loginPacketSize;
        m_packetNodes[nodeIdx].isDynamic = false;
        m_loginPool.enqueue(&m_packetNodes[nodeIdx]);
        nodeIdx++;
    }

    for(uint32 i = 0; i < m_config.textPoolSize; ++i) 
    {
        m_packetNodes[nodeIdx].payload = &m_textBuffer[i * m_config.textPacketSize];
        m_packetNodes[nodeIdx].maxBufferSize = m_config.textPacketSize;
        m_packetNodes[nodeIdx].isDynamic = false;
        m_textPool.enqueue(&m_packetNodes[nodeIdx]);
        nodeIdx++;
    }

    for(uint32 i = 0; i < m_config.gamePoolSize; ++i) 
    {
        m_packetNodes[nodeIdx].payload = &m_gameBuffer[i * m_config.gamePacketSize];
        m_packetNodes[nodeIdx].maxBufferSize = m_config.gamePacketSize;
        m_packetNodes[nodeIdx].isDynamic = false;
        m_gamePool.enqueue(&m_packetNodes[nodeIdx]);
        nodeIdx++;
    }
}

/**
 * todo here only allow dynamic for item data
 */

PooledPacket* PacketPool::Acquire(uint32 size, bool allowDynamic)
{
    PooledPacket* pPacket = nullptr;

    if(size <= m_config.gamePacketSize) 
    {
        if(m_gamePool.try_dequeue(pPacket)) return pPacket;
    }
    else if(size <= m_config.textPacketSize) 
    {
        if(m_textPool.try_dequeue(pPacket)) return pPacket;
    }
    else if(size <= m_config.loginPacketSize) 
    {
        if(m_loginPool.try_dequeue(pPacket)) return pPacket;
    }

    if(!allowDynamic)
        return nullptr;

    pPacket = new PooledPacket();
    pPacket->payload = new uint8[size];
    pPacket->maxBufferSize = size;
    pPacket->isDynamic = true;

    return pPacket;
}

void PacketPool::Release(PooledPacket* pPacket)
{
    if(!pPacket)
        return;

    if(pPacket->isDynamic) 
    {
        SAFE_DELETE_ARRAY(pPacket->payload)
        SAFE_DELETE(pPacket);
        return;
    }

    uint32 originalSize = pPacket->maxBufferSize;
    pPacket->dataLength = 0;

    if(originalSize == m_config.gamePacketSize) m_gamePool.enqueue(pPacket);
    else if(originalSize == m_config.textPacketSize) m_textPool.enqueue(pPacket);
    else if(originalSize == m_config.loginPacketSize) m_loginPool.enqueue(pPacket);
}