#include "NetPacket.h"
#include "../Proton/ProtonUtils.h"

moodycamel::ConcurrentQueue<NetworkEvent> gPacketOutgoingQueue;

void SendUDPPacketRaw(uint32 netID, eMessagePacketType msgType, void* pData, uint32 dataSize, uint8* pExtraData)
{
    if(msgType == NET_MESSAGE_GAME_PACKET && ((GameUpdatePacket*)pData)->HasFlag(GAME_PACKET_FLAG_EXTENDED_DATA)) 
    {
        if(!pExtraData)
            return;

        uint32 totalSize = dataSize + 5 + ((GameUpdatePacket*)pData)->extraDataSize;

        PooledPacket* pPoolPacket = gPacketPool.Acquire(totalSize, true);
        if(!pPoolPacket) 
            return;

        NetworkEvent netEvent;
        netEvent.netID = netID;
        netEvent.pPacket = pPoolPacket;
        netEvent.pPacket->dataLength = totalSize;

        uint8* pCur = pPoolPacket->payload;
    
        std::memcpy(pCur, &msgType, 4); pCur += 4;
        std::memcpy(pCur, pData, dataSize); pCur += dataSize;
        std::memcpy(pCur, pExtraData, ((GameUpdatePacket*)pData)->extraDataSize);

        gPacketOutgoingQueue.enqueue(std::move(netEvent));
    }
    else 
    {
        uint32 totalSize = dataSize + 5;

        PooledPacket* pPoolPacket = gPacketPool.Acquire(totalSize, true);
        if(!pPoolPacket) 
            return;

        NetworkEvent netEvent;
        netEvent.netID = netID;
        netEvent.pPacket = pPoolPacket;
        netEvent.pPacket->dataLength = totalSize;

        uint8* pCur = pPoolPacket->payload;
    
        std::memcpy(pCur, &msgType, 4); pCur += 4;
        std::memcpy(pCur, pData, dataSize); pCur += dataSize;

        gPacketOutgoingQueue.enqueue(std::move(netEvent));
    }
}

void SendUDPPacket(uint32 netID, eMessagePacketType messageType, const char* message, uint32 dataSize)
{
    if(!message)
        return;

    if(dataSize == 0)
    {
        dataSize = strlen(message);
    }

    uint32 totalSize = dataSize + 5;

    PooledPacket* pPoolPacket = gPacketPool.Acquire(totalSize, true);
    if(!pPoolPacket) 
        return;

    NetworkEvent netEvent;
    netEvent.netID = netID;
    netEvent.pPacket = pPoolPacket;
    netEvent.pPacket->dataLength = totalSize;

    uint8* pCur = pPoolPacket->payload;

    std::memcpy(pCur, &messageType, 4); pCur += 4;
    std::memcpy(pCur, message, dataSize); pCur += dataSize;
    gPacketOutgoingQueue.enqueue(std::move(netEvent));
}

void SendUDPDisconnectPacket(uint32 netID)
{
    NetworkEvent netEvent;
    netEvent.netID = netID;
    netEvent.shouldDisconnect = true;

    gPacketOutgoingQueue.enqueue(std::move(netEvent));
}

void SendCallFunctionPacket(uint32 senderNetID, const VariantVector& data, int32 netID, int32 delay)
{
    if(data.empty())
        return;

    uint32 extraSize = Proton::GetMemEstiamte(data);
    uint32 totalSize = sizeof(GameUpdatePacket) + 5 + extraSize;

    PooledPacket* pPoolPacket = gPacketPool.Acquire(totalSize, true);
    if(!pPoolPacket) 
        return;

    pPoolPacket->dataLength = totalSize;

    GameUpdatePacket gamePacket;
    gamePacket.type = NET_GAME_PACKET_CALL_FUNCTION;
    gamePacket.flags |= GAME_PACKET_FLAG_EXTENDED_DATA;
    gamePacket.field_4 = netID;
    gamePacket.field_7 = delay;
    gamePacket.extraDataSize = extraSize;

    uint8* pCur = pPoolPacket->payload;
    uint32 msgType = NET_MESSAGE_GAME_PACKET;
    
    std::memcpy(pCur, &msgType, 4); pCur += 4;
    std::memcpy(pCur, &gamePacket, sizeof(GameUpdatePacket)); pCur += sizeof(GameUpdatePacket);

    uint32 writtenSize = 0;
    Proton::SerializeToMem(data, &writtenSize, pCur);

    NetworkEvent netEvent{ ENET_EVENT_TYPE_RECEIVE, senderNetID, pPoolPacket };
    gPacketOutgoingQueue.enqueue(std::move(netEvent));
}

void SendCallFunctionPacket(uint32 senderNetID, uint8* pExtraData, uint32 extraSize, int32 netID, int32 delay)
{
    if(!pExtraData || extraSize == 0)
        return;

    uint32 totalSize = sizeof(GameUpdatePacket) + 5 + extraSize;

    PooledPacket* pPoolPacket = gPacketPool.Acquire(totalSize, true);
    if(!pPoolPacket) 
        return;

    pPoolPacket->dataLength = totalSize;

    GameUpdatePacket gamePacket;
    gamePacket.type = NET_GAME_PACKET_CALL_FUNCTION;
    gamePacket.flags |= GAME_PACKET_FLAG_EXTENDED_DATA;
    gamePacket.field_4 = netID;
    gamePacket.field_7 = delay;
    gamePacket.extraDataSize = extraSize;

    uint8* pCur = pPoolPacket->payload;
    uint32 msgType = NET_MESSAGE_GAME_PACKET;
    
    std::memcpy(pCur, &msgType, 4); pCur += 4;
    std::memcpy(pCur, &gamePacket, sizeof(GameUpdatePacket)); pCur += sizeof(GameUpdatePacket);
    std::memcpy(pCur, pExtraData, extraSize);

    NetworkEvent netEvent{ ENET_EVENT_TYPE_RECEIVE, senderNetID, pPoolPacket };
    gPacketOutgoingQueue.enqueue(std::move(netEvent));
}

/*bool SendENetPacketRaw(eMessagePacketType messageType, void *pData, uint32 dataSize, uint8 *pExtraData, ENetPeer *pPeer)
{
    if(!pPeer) {
        return false;
    }

    if(messageType == NET_MESSAGE_GAME_PACKET && ((GameUpdatePacket*)pData)->flags & GAME_PACKET_FLAG_EXTENDED_DATA) {
        ENetPacket* pPacket = enet_packet_create(nullptr, dataSize + 5 + ((GameUpdatePacket*)pData)->extraDataSize, ENET_PACKET_FLAG_RELIABLE);
        
        memcpy(pPacket->data, &messageType, 4);
        memcpy(pPacket->data + 4, pData, dataSize);
        memcpy(pPacket->data + 4 + dataSize, pExtraData, ((GameUpdatePacket*)pData)->extraDataSize);

        if(enet_peer_send(pPeer, 0, pPacket) != 0) {
            enet_packet_destroy(pPacket);
            return false;
        }
    }
    else {
        ENetPacket* pPacket = enet_packet_create(nullptr, dataSize + 5, ENET_PACKET_FLAG_RELIABLE);
        memcpy(pPacket->data, &messageType, 4);
        memcpy(pPacket->data + 4, pData, dataSize);

        if(enet_peer_send(pPeer, 0, pPacket) != 0) {
            enet_packet_destroy(pPacket);
            return false;
        }
    }

    return true;
}

bool SendENetPacket(eMessagePacketType messageType, const char *message, ENetPeer *pPeer)
{
    if(!pPeer) {
        return false;
    }

    ENetPacket* pPacket = enet_packet_create(nullptr, 5 + strlen(message), ENET_PACKET_FLAG_RELIABLE);

    memset(pPacket->data, 0, 5 + strlen(message));
    memcpy(pPacket->data, &messageType, 4);
    memcpy(pPacket->data + 4, message, strlen(message));

    if(enet_peer_send(pPeer, 0, pPacket) != 0) {
        enet_packet_destroy(pPacket);
        return false;
    }

    return true;
}*/

const char* GetTextFromEnetPacket(uint8* pData, uint32 dataLength)
{
    if(!pData || dataLength < 4)
        return "";

    if(pData[dataLength - 1] != 0)
        pData[dataLength - 1] = 0;

    return (const char*)pData + 4;
}

uint32 GetMessageTypeFromEnetPacket(uint8* pData, uint32 dataLength)
{
    if(dataLength < 4)
        return 0;

    return *(uint32*)pData;
}

GameUpdatePacket* GetGamePacketFromEnetPacket(uint8* pData, uint32 dataLength)
{
    if(dataLength < sizeof(GameUpdatePacket))
        return nullptr;

    GameUpdatePacket* pGamePacket = (GameUpdatePacket*)(pData + 4);
    if(!(pGamePacket->flags & GAME_PACKET_FLAG_EXTENDED_DATA)) 
    {
        pGamePacket->extraDataSize = 0;
    }
    else if(dataLength < pGamePacket->extraDataSize + sizeof(pGamePacket))
        return nullptr;
    
    return pGamePacket;
}

uint8* GetExtendedDataFromGamePacket(GameUpdatePacket* pUpdatePacket)
{
    if(!(pUpdatePacket->flags & GAME_PACKET_FLAG_EXTENDED_DATA))
        return nullptr;

    return (uint8*)pUpdatePacket + sizeof(GameUpdatePacket);
}
