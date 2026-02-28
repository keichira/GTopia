#include "NetPacket.h"

#include "../IO/File.h"
#include "../IO/Log.h"

bool SendENetPacketRaw(eMessagePacketType messageType, void* pData, uint32 dataSize, uint8* pExtraData, ENetPeer* pPeer)
{
    if(!pPeer) {
        return false;
    }

    if(messageType == NET_MESSAGE_GAME_PACKET && ((GameUpdatePacket*)pData)->flags & NET_GAME_PACKET_FLAGS_EXTENDED) {
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
}

const char* GetTextFromEnetPacket(ENetPacket* pPacket)
{
    if(!pPacket || pPacket->dataLength < 4) {
        return "";
    }

    if (pPacket->data[pPacket->dataLength - 1] != 0)
        pPacket->data[pPacket->dataLength - 1] = 0;

    return (const char*)pPacket->data + 4;
}

uint32 GetMessageTypeFromEnetPacket(ENetPacket* pPacket)
{
    if(pPacket->dataLength < 4) {
        return 0;
    }

    return *(uint32*)pPacket->data;
}

GameUpdatePacket* GetGamePacketFromEnetPacket(ENetPacket* pPacket)
{
    if(pPacket->dataLength < sizeof(GameUpdatePacket)) {
        return nullptr;
    }

    GameUpdatePacket* pGamePacket = (GameUpdatePacket*)(pPacket->data + 4);
    if(!(pGamePacket->flags & NET_GAME_PACKET_FLAGS_EXTENDED)) {
        pGamePacket->extraDataSize = 0;
    }
    else if(pPacket->dataLength < pGamePacket->extraDataSize + sizeof(pGamePacket)) {
        return nullptr;
    }
    
    return pGamePacket;
}

uint8* GetExtendedDataFromGamePacket(GameUpdatePacket* pUpdatePacket)
{
    if(!(pUpdatePacket->flags & NET_GAME_PACKET_FLAGS_EXTENDED)) {
        return nullptr;
    }

    return (uint8*)pUpdatePacket + sizeof(GameUpdatePacket);
}
