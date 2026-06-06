#pragma once

#include "../Precompiled.h"
#include "GamePacket.h"
#include "PacketPool.h"
#include <concurrentqueue.h>
#include <enet/enet.h>

struct NetworkEvent
{
    ENetEventType type = ENET_EVENT_TYPE_NONE;
    uint32 netID = 0;
    PooledPacket* pPacket = nullptr;
    uint32 host = 0;
    bool shouldDisconnect = false;
};

extern moodycamel::ConcurrentQueue<NetworkEvent> gPacketOutgoingQueue;

void SendUDPPacketRaw(uint32 netID, eMessagePacketType msgType, void* pData, uint32 dataSize, uint8* pExtraData);
void SendUDPPacket(uint32 netID, eMessagePacketType messageType, const char* message, uint32 dataSize = 0);
void SendUDPDisconnectPacket(uint32 netID);

void SendCallFunctionPacket(uint32 senderNetID, const VariantVector& data, int32 netID = -1, int32 delay = -1);
void SendCallFunctionPacket(uint32 senderNetID, uint8* pExtraData, uint32 extraSize, int32 netID = -1, int32 delay = -1);

//bool SendENetPacketRaw(eMessagePacketType messageType, void* pData, uint32 dataSize, uint8* pExtraData, ENetPeer* pPeer);
//bool SendENetPacket(eMessagePacketType messageType, const char* message, ENetPeer* pPeer);

const char* GetTextFromEnetPacket(uint8* pData, uint32 dataLength);
uint32 GetMessageTypeFromEnetPacket(uint8* pData, uint32 dataLength);
GameUpdatePacket* GetGamePacketFromEnetPacket(uint8* pData, uint32 dataLength);
uint8* GetExtendedDataFromGamePacket(GameUpdatePacket* pUpdatePacket);