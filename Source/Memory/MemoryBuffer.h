#pragma once

#include "../Precompiled.h"

class MemoryBuffer
{
public:
    MemoryBuffer();

    explicit MemoryBuffer(uint32 size);

    MemoryBuffer(void* pData, uint32 size);
    MemoryBuffer(const void* pData, uint32 size);

    ~MemoryBuffer();

public:
    void Destroy();

    uint8* GetData() { return m_pBuffer; }
    const uint8* GetData() const { return m_pBuffer; }
    uint32 GetOffset() const { return m_pos; }
    uint32 GetBufferSize() const { return m_bufferSize; }
    bool IsCountOnly() const { return m_countOnly; }

    uint32 Seek(uint32 position);

    template <typename T> uint32 Read(T& data) { return ReadRaw(&data, sizeof(T)); }
    template <typename T> uint32 Write(const T& data) { return WriteRaw(&data, sizeof(T)); }

    template <typename T> uint32 ReadWrite(T& data, bool write)
    {
        return write ? WriteRaw(&data, sizeof(T)) : ReadRaw(&data, sizeof(T));
    }

    uint32 ReadWriteString(string& data, bool write) { return write ? WriteStringRaw(data) : ReadStringRaw(data); }

    uint32 ReadWriteRaw(void* data, uint32 size, bool write)
    {
        return write ? WriteRaw(data, size) : ReadRaw(data, size);
    }

    uint32 ReadRaw(void* pDest, uint32 size);
    uint32 WriteRaw(const void* pData, uint32 size);
    uint32 ReadStringRaw(string& pDest);
    uint32 WriteStringRaw(const string& pData);

private:
    uint8* m_pBuffer;
    uint32 m_bufferSize;
    uint32 m_pos;

    bool m_ableToWrite;
    bool m_countOnly;
    bool m_ownsMemory;
};