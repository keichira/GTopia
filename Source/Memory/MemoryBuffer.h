#pragma once

#include "../Precompiled.h"

class MemoryBuffer {
public:
    MemoryBuffer();
    MemoryBuffer(void* pData, uint32 size);
    MemoryBuffer(const void* pData, uint32 size);
    MemoryBuffer(std::vector<uint8>& data);
    MemoryBuffer(const std::vector<uint8>& data);

public:
    uint32 Seek(uint32 position);
    uint32 GetOffset() const { return m_pos; }

    template<typename T>
    uint32 Read(T& data)
    {
        return ReadRaw(&data, sizeof(T));
    }

    template<typename T>
    uint32 Write(const T& data)
    {
        return WriteRaw(&data, sizeof(T));
    }

    template<typename T>
    uint32 ReadWrite(T& data, bool write)
    {
        if (write) {
            return WriteRaw(&data, sizeof(T));
        }
        else {
            return ReadRaw(&data, sizeof(T));
        }
    }

    uint32 ReadWriteString(string& data, bool write)
    {
        if (write) {
            return WriteStringRaw(data);
        }
        else {
            return ReadStringRaw(data);
        }
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
};