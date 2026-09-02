#pragma once

#include "../Memory/MemoryBuffer.h"
#include "../Precompiled.h"

#pragma pack(push, 1)
struct TCPPacketHeader
{
    uint32 bodySize;
    uint16 packetID;
    uint8 flags;
    uint8 reserved;
};
#pragma pack(pop)

enum eTCPPacketFlags : uint8
{
    TCP_PACKET_FLAG_STREAM = 1 << 0,
    TCP_PACKET_FLAG_POD = 1 << 1
};

class TCPPacketReader;
class NetClient;

struct TCPPacketEvent
{
    NetClient* pClient = nullptr;
    TCPPacketHeader header;
    std::vector<uint8> payload;
    uint64 reqTime = 0;
};

class TCPPacketWriter
{
public:
    TCPPacketWriter(uint16 packetID, uint32 expectedSize = 128, uint8 flags = TCP_PACKET_FLAG_STREAM)
        : m_buffer(sizeof(TCPPacketHeader) + expectedSize)
    {
        TCPPacketHeader header;
        header.bodySize = 0;
        header.packetID = packetID;
        header.flags = flags;
        header.reserved = 0;

        m_buffer.Write(header);
    }

public:
    template <typename T> bool Write(const T& data)
    {
        EnsureCapacity(sizeof(T));
        return m_buffer.Write(data) == sizeof(T);
    }

    bool WriteString(const string& str)
    {
        uint16 len = (uint16)(str.length());
        uint32 requiredSize = sizeof(uint16) + len;

        EnsureCapacity(requiredSize);

        m_buffer.WriteStringRaw(str);
        return true;
    }

    template <typename T> bool WriteArray(const std::vector<T>& vec)
    {
        uint32 count = (uint32)(vec.size());
        uint32 requiredSize = sizeof(uint32) + (count * sizeof(T));

        EnsureCapacity(requiredSize);

        m_buffer.Write(count);
        if (count > 0)
        {
            m_buffer.WriteRaw(vec.data(), count * sizeof(T));
        }
        return true;
    }

    bool WriteRaw(const void* pData, uint32 size)
    {
        if (!pData || size == 0)
            return false;

        EnsureCapacity(size);
        return m_buffer.WriteRaw(pData, size) == size;
    }

    template <typename T> bool WritePOD(const T& podData) { return Write<T>(podData); }

    uint8* Finalize(uint32& outTotalSize)
    {
        outTotalSize = m_buffer.GetOffset();
        uint32 bodySize = outTotalSize - sizeof(TCPPacketHeader);

        TCPPacketHeader* pHeader = (TCPPacketHeader*)(m_buffer.GetData());
        pHeader->bodySize = bodySize;

        return m_buffer.GetData();
    }

    MemoryBuffer& GetBuffer() { return m_buffer; }

private:
    inline void EnsureCapacity(uint32 additionalBytes)
    {
        m_buffer.Realloc((m_buffer.GetBufferSize() + additionalBytes) * 2);
    }

private:
    MemoryBuffer m_buffer;
};

class TCPPacketReader
{
public:
    TCPPacketReader(const void* pData, uint32 size) : m_buffer(pData, size) {}

public:
    // const TCPPacketHeader& GetHeader() const { return m_header; }

    template <typename T> bool Read(T& outData) { return m_buffer.Read(outData) == sizeof(T); }

    bool ReadString(string& outStr) { return m_buffer.ReadStringRaw(outStr) > 0; }

    template <typename T> bool ReadArray(std::vector<T>& outElements)
    {
        outElements.clear();
        uint32 count = 0;
        if (!Read(count) || count == 0)
            return false;

        outElements.resize(count);
        uint32 totalBytes = count * sizeof(T);
        return m_buffer.ReadRaw(outElements.data(), totalBytes) == totalBytes;
    }

    bool ReadRaw(void* pDest, uint32 size)
    {
        if (!pDest || size == 0)
            return false;
        return m_buffer.ReadRaw(pDest, size) == size;
    }

    template <typename T> bool ReadPOD(T& outPod) { return ReadRaw(&outPod, sizeof(T)); }

private:
    MemoryBuffer m_buffer;
    /// TCPPacketHeader m_header;
};