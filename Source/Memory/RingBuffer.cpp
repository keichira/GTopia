#include "RingBuffer.h"
#include "../Math/Math.h"

RingBuffer::RingBuffer(int32 size)
: m_readIndex(0), m_writeIndex(0), m_size(0)
{
    m_data.resize(size);
}

RingBuffer::~RingBuffer()
{
}

uint32 RingBuffer::Write(void* pData, uint32 size)
{
    if(size == 0) {
        return 0;
    }

    uint32 space = m_data.capacity() - m_size;
    uint32 writeSize = Min(size, space);

    if(writeSize == 0) {
        return 0;
    }

    uint32 firstSize = Min(writeSize, m_data.capacity() - m_writeIndex);
    memcpy(&m_data[m_writeIndex], pData, firstSize);

    m_writeIndex = (m_writeIndex + firstSize) % m_data.capacity();
    m_size += firstSize;

    uint32 totalWritten = firstSize;
    uint32 remainSize = writeSize - firstSize;
    if(writeSize > 0) {
        totalWritten +=  Write((uint8*)pData + firstSize, remainSize);
    }

    return totalWritten;
}

uint32 RingBuffer::Read(void* pDest, uint32 size)
{
    if(size == 0 || m_size == 0) {
        return 0;
    }

    uint32 readSize = Min(size, m_size);

    uint32 firstSize = Min(readSize, m_data.capacity() - m_readIndex);
    memcpy(pDest, &m_data[m_readIndex], firstSize);

    m_readIndex = (m_readIndex + firstSize) % m_data.capacity();
    m_size -= firstSize;

    uint32 totalRead = firstSize;
    uint32 remainSize = readSize - firstSize;
    if(remainSize > 0) {
        totalRead += Read((uint8*)pDest + firstSize, remainSize);
    }

    return totalRead;
}

uint32 RingBuffer::Peek(void* pDest, uint32 size)
{
    if (size == 0 || m_size == 0) {
        return 0;
    }

    uint32 peekSize = Min(size, m_size);

    uint32 firstSize = Min(peekSize, m_data.capacity() - m_readIndex);
    memcpy(pDest, &m_data[m_readIndex], firstSize);

    uint32 totalPeeked = firstSize;
    uint32 remainSize = peekSize - firstSize;

    if (remainSize > 0) {
        memcpy((uint8*)pDest + firstSize, &m_data[0], remainSize);
        totalPeeked += remainSize;
    }

    return totalPeeked;
}

void RingBuffer::Skip(uint32 size)
{
    m_readIndex = (m_readIndex + size) % m_data.capacity();
    m_size -= size;
}
