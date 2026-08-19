#include "MemoryBuffer.h"

MemoryBuffer::MemoryBuffer()
    : m_pBuffer(nullptr), m_bufferSize(0), m_pos(0), m_ableToWrite(false), m_countOnly(true), m_ownsMemory(false)
{
}

MemoryBuffer::MemoryBuffer(uint32 size)
    : m_bufferSize(size), m_pos(0), m_ableToWrite(true), m_countOnly(false), m_ownsMemory(true)
{
    m_pBuffer = (size > 0) ? new uint8[size]() : nullptr;
}

MemoryBuffer::MemoryBuffer(void* pData, uint32 size)
    : m_pBuffer((uint8*)(pData)), m_bufferSize(size), m_pos(0), m_ableToWrite(true), m_countOnly(false),
      m_ownsMemory(false)
{
}

MemoryBuffer::MemoryBuffer(const void* pData, uint32 size)
    : m_pBuffer((uint8*)(pData)), m_bufferSize(size), m_pos(0), m_ableToWrite(false), m_countOnly(false),
      m_ownsMemory(false)
{
}

MemoryBuffer::~MemoryBuffer()
{
    Destroy();
}

void MemoryBuffer::Destroy()
{
    if (m_ownsMemory)
    {
        SAFE_DELETE_ARRAY(m_pBuffer);
    }
    m_bufferSize = 0;
    m_pos = 0;
    m_ownsMemory = false;
}

uint32 MemoryBuffer::ReadRaw(void* pDest, uint32 size)
{
    if (size == 0)
        return 0;

    if (m_countOnly)
    {
        m_pos += size;
        return size;
    }

    if (!pDest || m_pos + size > m_bufferSize)
        return 0;

    memcpy(pDest, m_pBuffer + m_pos, size);
    m_pos += size;
    return size;
}

uint32 MemoryBuffer::WriteRaw(const void* pData, uint32 size)
{
    if (size == 0)
        return 0;

    if (m_countOnly)
    {
        m_pos += size;
        return size;
    }

    if (!m_ableToWrite || !pData || m_pos + size > m_bufferSize)
        return 0;

    memcpy(m_pBuffer + m_pos, pData, size);
    m_pos += size;
    return size;
}

uint32 MemoryBuffer::ReadStringRaw(string& pDest)
{
    uint16 strLen = 0;
    if (Read(strLen) == 0)
        return 0;

    if (m_countOnly)
    {
        m_pos += strLen;
        return strLen + sizeof(uint16);
    }

    if (m_pos + strLen > m_bufferSize)
        return 0;

    pDest.resize(strLen);
    if (strLen > 0)
    {
        memcpy(pDest.data(), m_pBuffer + m_pos, strLen);
        m_pos += strLen;
    }

    return strLen + sizeof(uint16);
}

uint32 MemoryBuffer::WriteStringRaw(const string& pData)
{
    uint16 strLen = (uint16)(pData.size());

    if (m_countOnly)
    {
        m_pos += sizeof(uint16) + strLen;
        return strLen + sizeof(uint16);
    }

    WriteRaw(&strLen, sizeof(uint16));
    if (strLen > 0)
    {
        WriteRaw(pData.data(), strLen);
    }

    return strLen + sizeof(uint16);
}

uint32 MemoryBuffer::Seek(uint32 position)
{
    if (m_countOnly)
    {
        m_pos = position;
        return m_pos;
    }

    m_pos = (position > m_bufferSize) ? m_bufferSize : position;
    return m_pos;
}