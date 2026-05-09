#pragma once

#include "../Memory/RingBuffer.h"
#include "../Utils/Variant.h"
#include "../Memory/MemoryBuffer.h"
#include <mutex>

#ifdef SOCKET_USE_TLS
    #include <openssl/ssl.h>
#endif

#define SOCKET_MAX_BUFFER_SIZE 4096

#ifdef _WIN32
    #include <winsock2.h>
    typedef SOCKET socket_t;
    #define SOCKET_INVALID INVALID_SOCKET
#else
    typedef int socket_t;
    #define SOCKET_INVALID (-1)
#endif

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
    socket_t socket = SOCKET_INVALID;
    int16 connectionID;
    eSocketClientStatus status = SOCKET_CLIENT_UNKNOWN;
    string ip;

    RingBuffer sendQueue = RingBuffer(8 * 1024);
    
#ifdef CLI_MEM_ITEM_MANAGER
    RingBuffer recvQueue = RingBuffer(50 * 16 * 1024);
#else
    RingBuffer recvQueue = RingBuffer(8 * 1024);
#endif

    NetSocket* pNetSocket = nullptr;
    void* data = nullptr;

    std::mutex recvMutex;
    std::mutex sendMutex;

#ifdef SOCKET_USE_TLS
    SSL* pSsl = nullptr;
#endif

    bool Send(const VariantVector& data);
    bool Send(void* pData, uint32 size);
};