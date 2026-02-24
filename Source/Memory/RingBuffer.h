#pragma once

#include "../Precompiled.h"

class RingBuffer {
public:
    RingBuffer(int32 size);
    ~RingBuffer();

public:
    uint32 Write(void* pData, uint32 size);
    uint32 Read(void* pDest, uint32 size);

    uint32 Peek(void* pDest, uint32 size);
    void Skip(uint32 size);

    uint32 GetAvailableSpace() { return m_data.capacity() - m_size; }
    uint32 GetDataSize() { return m_size; }

    int32 GetWriteIndex() const { return m_writeIndex; }

private:
    std::vector<uint8> m_data;
    int32 m_readIndex;
    int32 m_writeIndex;
    int32 m_size;
};