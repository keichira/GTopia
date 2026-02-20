#pragma once

#include "../Memory/RingBuffer.h"
#include "../Utils/Variant.h"
#include "../Memory/MemoryBuffer.h"
#include <mutex>

#define SOCKET_MAX_BUFFER_SIZE 4096

class NetSocket;

enum eSocketClientStatus 
{
    SOCKET_CLIENT_UNKNOWN,
    SOCKET_CLIENT_CONNECTED,
    SOCKET_CLIENT_CONNECTING,
    SOCKET_CLIENT_DISCONNECTED,
    SOCKET_CLIENT_CLOSE
};

uint8* SerializeVariantVectorForTCP(const VariantVector& varVector, uint32& outSize);
void DeSerializeVariantVectorForTCP(MemoryBuffer& memBuffer, VariantVector& out);

struct NetClient 
{
    int32 socket = -1;
    int16 connectionID;
    eSocketClientStatus status = SOCKET_CLIENT_UNKNOWN;

    RingBuffer sendQueue = RingBuffer(SOCKET_MAX_BUFFER_SIZE/2);
    RingBuffer recvQueue = RingBuffer(SOCKET_MAX_BUFFER_SIZE/2);

    NetSocket* pNetSocket = nullptr;
    void* data = nullptr;

    std::mutex mutex;

    bool Send(const VariantVector& data);
};