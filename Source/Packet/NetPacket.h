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
    ENetPacket* pENetPacket = nullptr; // to fix double allocations
    PooledPacket* pPacket = nullptr;   // for incoming pooled packets
    uint32 host = 0;

    std::vector<uint32> targets;

    bool shouldDisconnect = false;
    bool isItemData = false;
};

extern moodycamel::ConcurrentQueue<NetworkEvent> gPacketOutgoingQueue;

void SendUDPPacketRaw(uint32 netID, eMessagePacketType msgType, void* pData, uint32 dataSize, uint8* pExtraData);
void SendUDPPacket(uint32 netID, eMessagePacketType messageType, const char* message, uint32 dataSize = 0);
void SendUDPDisconnectPacket(uint32 netID);

void SendUDPItemDataPacket(uint32 netID, float gameVersion, uint32 platformType, GameUpdatePacket* pGamePacket);
void SendCallFunctionPacket(uint32 senderNetID, const VariantVector& data, int32 netID = -1, int32 delay = -1);
void SendCallFunctionPacket(uint32 senderNetID, uint8* pExtraData, uint32 extraSize, int32 netID = -1,
                            int32 delay = -1);

void SendUDPPacketRawMulti(std::vector<uint32>&& targets, eMessagePacketType msgType, void* pData, uint32 dataSize,
                           uint8* pExtraData);
void SendUDPPacketMulti(std::vector<uint32>&& targets, eMessagePacketType msgType, const char* message,
                        uint32 dataSize = 0);
void SendCallFunctionPacketMulti(std::vector<uint32>&& targets, const VariantVector& data, int32 netID = -1,
                                 int32 delay = -1);
void SendCallFunctionPacketMulti(std::vector<uint32>&& targets, uint8* pExtraData, uint32 extraSize, int32 netID = -1,
                                 int32 delay = -1);

// do not use due thread safety
/*bool SendENetPacketRaw(eMessagePacketType messageType, void* pData, uint32 dataSize, uint8* pExtraData,
                       ENetPeer* pPeer);
bool SendENetPacket(eMessagePacketType messageType, const char* message, ENetPeer* pPeer);*/

const char* GetTextFromEnetPacket(uint8* pData, uint32 dataLength);
uint32 GetMessageTypeFromEnetPacket(uint8* pData, uint32 dataLength);
GameUpdatePacket* GetGamePacketFromEnetPacket(uint8* pData, uint32 dataLength, bool checkExtra = true);
uint8* GetExtendedDataFromGamePacket(GameUpdatePacket* pUpdatePacket);