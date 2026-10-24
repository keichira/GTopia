#include "NetPacket.h"
#include "../Proton/ProtonUtils.h"
#include "../Utils/Timer.h"

moodycamel::ConcurrentQueue<NetworkEvent> gPacketOutgoingQueue;

void SendUDPDisconnectPacket(uint32 netID)
{
    NetworkEvent netEvent;
    netEvent.netID = netID;
    netEvent.shouldDisconnect = true;

    gPacketOutgoingQueue.enqueue(std::move(netEvent));
}

void SendUDPItemDataPacket(uint32 netID, float gameVersion, uint32 platformType, GameUpdatePacket* pGamePacket)
{
    if (!pGamePacket)
        return;

    pGamePacket->field_10 = gameVersion;
    pGamePacket->field_11 = platformType;

    uint32 totalSize = sizeof(GameUpdatePacket) + 5;
    PooledPacket* pPoolPacket = gPacketPool.Acquire(totalSize);
    if (!pPoolPacket)
        return;

    pPoolPacket->dataLength = totalSize;
    uint8* pCursor = pPoolPacket->payload;
    uint32 messageType = NET_MESSAGE_GAME_PACKET;

    memcpy(pCursor, &messageType, 4);
    pCursor += 4;
    memcpy(pCursor, pGamePacket, sizeof(GameUpdatePacket));
    pCursor += sizeof(GameUpdatePacket);

    NetworkEvent netEvent;
    netEvent.netID = netID;
    netEvent.isItemData = true;
    netEvent.pPacket = pPoolPacket;

    gPacketOutgoingQueue.enqueue(std::move(netEvent));
}

static ENetPacket* BuildRaw(eMessagePacketType messageType, void* pData, uint32 dataSize, uint8* pExtraData)
{
    if (!pData || dataSize == 0)
        return nullptr;

    uint32 extraSize = 0;
    if (messageType == NET_MESSAGE_GAME_PACKET)
    {
        GameUpdatePacket* pGamePacket = (GameUpdatePacket*)pData;
        if (pGamePacket->flags & GAME_PACKET_FLAG_EXTENDED_DATA)
            extraSize = pGamePacket->extraDataSize;
    }
    if (extraSize > 0 && !pExtraData)
        return nullptr;

    ENetPacket* pPacket = enet_packet_create(nullptr, dataSize + 5 + extraSize, ENET_PACKET_FLAG_RELIABLE);
    if (!pPacket)
        return nullptr;

    uint8* pCursor = pPacket->data;
    memcpy(pCursor, &messageType, 4);
    pCursor += 4;
    memcpy(pCursor, pData, dataSize);
    pCursor += dataSize;
    if (extraSize > 0)
    {
        memcpy(pCursor, pExtraData, extraSize);
        pCursor += extraSize;
    }
    *pCursor = 0;
    return pPacket;
}

static ENetPacket* BuildText(eMessagePacketType messageType, const char* message, uint32 dataSize)
{
    if (!message)
        return nullptr;

    if (dataSize == 0)
        dataSize = (uint32)strlen(message);

    ENetPacket* pPacket = enet_packet_create(nullptr, dataSize + 5, ENET_PACKET_FLAG_RELIABLE);
    if (!pPacket)
        return nullptr;

    uint8* pCursor = pPacket->data;
    memcpy(pCursor, &messageType, 4);
    pCursor += 4;
    memcpy(pCursor, message, dataSize);
    pCursor += dataSize;
    *pCursor = 0;
    return pPacket;
}

static ENetPacket* BuildCallFunction(uint8* pExtraData, uint32 extraSize, int32 netID, int32 delay)
{
    if (!pExtraData || extraSize == 0)
        return nullptr;

    ENetPacket* pPacket =
        enet_packet_create(nullptr, sizeof(GameUpdatePacket) + 5 + extraSize, ENET_PACKET_FLAG_RELIABLE);
    if (!pPacket)
        return nullptr;

    GameUpdatePacket gamePacket;
    gamePacket.type = NET_GAME_PACKET_CALL_FUNCTION;
    gamePacket.flags |= GAME_PACKET_FLAG_EXTENDED_DATA;
    gamePacket.field_4 = netID;
    gamePacket.field_7 = delay;
    gamePacket.extraDataSize = extraSize;

    uint8* pCursor = pPacket->data;
    uint32 messageType = NET_MESSAGE_GAME_PACKET;
    memcpy(pCursor, &messageType, 4);
    pCursor += 4;
    memcpy(pCursor, &gamePacket, sizeof(GameUpdatePacket));
    pCursor += sizeof(GameUpdatePacket);
    memcpy(pCursor, pExtraData, extraSize);
    pCursor += extraSize;
    *pCursor = 0;
    return pPacket;
}

static ENetPacket* BuildCallFunction(const VariantVector& data, int32 netID, int32 delay)
{
    if (data.empty())
        return nullptr;

    uint32 extraSize = Proton::GetMemEstiamte(data);
    std::vector<uint8> buffer(extraSize);
    uint32 writtenSize = 0;
    Proton::SerializeToMem(data, &writtenSize, buffer.data());
    return BuildCallFunction(buffer.data(), extraSize, netID, delay);
}

void SendUDPPacketRaw(uint32 netID, eMessagePacketType messageType, void* pData, uint32 dataSize, uint8* pExtraData)
{
    ENetPacket* pPacket = BuildRaw(messageType, pData, dataSize, pExtraData);
    if (!pPacket)
        return;

    NetworkEvent event;
    event.netID = netID;
    event.pENetPacket = pPacket;
    gPacketOutgoingQueue.enqueue(std::move(event));
}

void SendUDPPacket(uint32 netID, eMessagePacketType messageType, const char* message, uint32 dataSize)
{
    ENetPacket* pPacket = BuildText(messageType, message, dataSize);
    if (!pPacket)
        return;

    NetworkEvent event;
    event.netID = netID;
    event.pENetPacket = pPacket;
    gPacketOutgoingQueue.enqueue(std::move(event));
}

void SendCallFunctionPacket(uint32 senderNetID, const VariantVector& data, int32 netID, int32 delay)
{
    ENetPacket* pPacket = BuildCallFunction(data, netID, delay);
    if (!pPacket)
        return;

    NetworkEvent event;
    event.netID = senderNetID;
    event.pENetPacket = pPacket;
    gPacketOutgoingQueue.enqueue(std::move(event));
}

void SendCallFunctionPacket(uint32 senderNetID, uint8* pExtraData, uint32 extraSize, int32 netID, int32 delay)
{
    ENetPacket* pPacket = BuildCallFunction(pExtraData, extraSize, netID, delay);
    if (!pPacket)
        return;

    NetworkEvent event;
    event.netID = senderNetID;
    event.pENetPacket = pPacket;
    gPacketOutgoingQueue.enqueue(std::move(event));
}

void SendUDPPacketRawMulti(NetTargets targets, eMessagePacketType messageType, void* pData, uint32 dataSize,
                           uint8* pExtraData)
{
    if (!targets || targets->empty())
        return;

    ENetPacket* pPacket = BuildRaw(messageType, pData, dataSize, pExtraData);
    if (!pPacket)
        return;

    NetworkEvent event;
    event.pENetPacket = pPacket;
    event.targets = std::move(targets);
    gPacketOutgoingQueue.enqueue(std::move(event));
}

void SendUDPPacketMulti(NetTargets targets, eMessagePacketType messageType, const char* message, uint32 dataSize)
{
    if (!targets || targets->empty())
        return;

    ENetPacket* pPacket = BuildText(messageType, message, dataSize);
    if (!pPacket)
        return;

    NetworkEvent event;
    event.pENetPacket = pPacket;
    event.targets = std::move(targets);
    gPacketOutgoingQueue.enqueue(std::move(event));
}

void SendCallFunctionPacketMulti(NetTargets targets, const VariantVector& data, int32 netID, int32 delay)
{
    if (!targets || targets->empty())
        return;

    ENetPacket* pPacket = BuildCallFunction(data, netID, delay);
    if (!pPacket)
        return;

    NetworkEvent event;
    event.pENetPacket = pPacket;
    event.targets = std::move(targets);
    gPacketOutgoingQueue.enqueue(std::move(event));
}

void SendCallFunctionPacketMulti(NetTargets targets, uint8* pExtraData, uint32 extraSize, int32 netID, int32 delay)
{
    if (!targets || targets->empty())
        return;

    ENetPacket* pPacket = BuildCallFunction(pExtraData, extraSize, netID, delay);
    if (!pPacket)
        return;

    NetworkEvent event;
    event.pENetPacket = pPacket;
    event.targets = std::move(targets);
    gPacketOutgoingQueue.enqueue(std::move(event));
}

/*bool SendENetPacketRaw(eMessagePacketType messageType, void* pData, uint32 dataSize, uint8* pExtraData, ENetPeer*
pPeer)
{
    if (!pPeer)
    {
        return false;
    }

    if (messageType == NET_MESSAGE_GAME_PACKET && ((GameUpdatePacket*)pData)->flags & GAME_PACKET_FLAG_EXTENDED_DATA)
    {
        ENetPacket* pPacket = enet_packet_create(nullptr, dataSize + 5 + ((GameUpdatePacket*)pData)->extraDataSize,
                                                 ENET_PACKET_FLAG_RELIABLE);

        memcpy(pPacket->data, &messageType, 4);
        memcpy(pPacket->data + 4, pData, dataSize);
        memcpy(pPacket->data + 4 + dataSize, pExtraData, ((GameUpdatePacket*)pData)->extraDataSize);

        if (enet_peer_send(pPeer, 0, pPacket) != 0)
        {
            enet_packet_destroy(pPacket);
            return false;
        }
    }
    else
    {
        ENetPacket* pPacket = enet_packet_create(nullptr, dataSize + 5, ENET_PACKET_FLAG_RELIABLE);
        memcpy(pPacket->data, &messageType, 4);
        memcpy(pPacket->data + 4, pData, dataSize);

        if (enet_peer_send(pPeer, 0, pPacket) != 0)
        {
            enet_packet_destroy(pPacket);
            return false;
        }
    }

    return true;
}*/

bool SendENetPacket(eMessagePacketType messageType, const char* message, ENetPeer* pPeer)
{
    if (!pPeer)
    {
        return false;
    }

    ENetPacket* pPacket = enet_packet_create(nullptr, 5 + strlen(message), ENET_PACKET_FLAG_RELIABLE);

    memset(pPacket->data, 0, 5 + strlen(message));
    memcpy(pPacket->data, &messageType, 4);
    memcpy(pPacket->data + 4, message, strlen(message));

    if (enet_peer_send(pPeer, 0, pPacket) != 0)
    {
        enet_packet_destroy(pPacket);
        return false;
    }

    return true;
}

const char* GetTextFromEnetPacket(uint8* pData, uint32 dataLength)
{
    if (!pData || dataLength < 4)
        return "";

    if (pData[dataLength - 1] != 0)
        pData[dataLength - 1] = 0;

    return (const char*)pData + 4;
}

uint32 GetMessageTypeFromEnetPacket(uint8* pData, uint32 dataLength)
{
    if (dataLength < 4)
        return 0;

    return *(uint32*)pData;
}

GameUpdatePacket* GetGamePacketFromEnetPacket(uint8* pData, uint32 dataLength, bool checkExtra)
{
    if (!pData || dataLength < 4 + sizeof(GameUpdatePacket))
        return nullptr;

    GameUpdatePacket* pGamePacket = (GameUpdatePacket*)(pData + 4);
    if (!(pGamePacket->flags & GAME_PACKET_FLAG_EXTENDED_DATA))
    {
        pGamePacket->extraDataSize = 0;
    }
    else if (checkExtra)
    {
        uint64 required = 4 + sizeof(GameUpdatePacket) + pGamePacket->extraDataSize;
        if (dataLength < required)
            return nullptr;
    }
    return pGamePacket;
}

uint8* GetExtendedDataFromGamePacket(GameUpdatePacket* pUpdatePacket)
{
    if (!(pUpdatePacket->flags & GAME_PACKET_FLAG_EXTENDED_DATA))
        return nullptr;

    return (uint8*)pUpdatePacket + sizeof(GameUpdatePacket);
}