#include "MemoryBuffer.h"

MemoryBuffer::MemoryBuffer(void* pData, uint32 size)
: m_pBuffer((uint8*)pData), m_bufferSize(size), m_pos(0), m_ableToWrite(true)
{
}

MemoryBuffer::MemoryBuffer(const void* pData, uint32 size)
: m_pBuffer((uint8*)pData), m_bufferSize(size), m_pos(0), m_ableToWrite(false)
{
}


MemoryBuffer::MemoryBuffer(std::vector<uint8>& data)
: m_pBuffer(data.data()), m_bufferSize(data.size()), m_pos(0), m_ableToWrite(true)
{
}

MemoryBuffer::MemoryBuffer(const std::vector<uint8>& data)
: m_pBuffer((uint8*)data.data()), m_bufferSize(data.size()), m_pos(0), m_ableToWrite(false)
{
}

uint32 MemoryBuffer::ReadRaw(void* pDest, uint32 size)
{
    if(size == 0 || m_pos + size > m_bufferSize) {
        return 0;
    }

    memcpy(pDest, m_pBuffer + m_pos, size);
    m_pos += size;

    return size;
}

uint32 MemoryBuffer::WriteRaw(const void* pData, uint32 size)
{
    if(!m_ableToWrite || !pData || size == 0 || m_pos + size > m_bufferSize) {
        return 0;
    }

    memcpy(m_pBuffer + m_pos, pData, size);
    m_pos += size;

    return size;
}

uint32 MemoryBuffer::ReadStringRaw(string& pDest, uint32 size)
{
    if(size == 0 || m_pos + size > m_bufferSize) {
        return 0;
    }

    uint32 strLen;
    if(ReadRaw(&strLen, size) != size) {
        return 0;
    }

    if(m_pos + strLen > m_bufferSize) {
        return 0;
    }

    pDest.resize(strLen);

    memcpy(pDest.data(), m_pBuffer + m_pos, strLen);
    m_pos += strLen;

    return strLen + size;
}

uint32 MemoryBuffer::WriteStringRaw(const string& pData, uint32 size)
{
    if(!m_ableToWrite || size == 0 || m_pos + size > m_bufferSize) {
        return 0;
    }

    uint32 strLen = pData.size();
    WriteRaw(&strLen, size);
    WriteRaw(pData.data(), strLen);

    return strLen + size;
}

uint32 MemoryBuffer::Seek(uint32 position)
{
    if(position > m_bufferSize) {
        position = m_bufferSize;
    }

    m_pos = position;
    return m_pos;
}
