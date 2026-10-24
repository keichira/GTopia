#pragma once

#include "../Precompiled.h"
#include "GamePacket.h"
#include "PacketPool.h"
#include <concurrentqueue.h>
#include <enet/enet.h>
#include <memory>

using NetTargets = std::shared_ptr<const std::vector<uint32>>;

struct NetworkEvent
{
    ENetEventType type = ENET_EVENT_TYPE_NONE;
    uint32 netID = 0;
    ENetPacket* pENetPacket = nullptr; // to fix double allocations
    PooledPacket* pPacket = nullptr;   // for incoming pooled packets
    uint32 host = 0;

    NetTargets targets;

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

void SendUDPPacketRawMulti(NetTargets targets, eMessagePacketType messageType, void* pData, uint32 dataSize,
                           uint8* pExtraData);
void SendUDPPacketMulti(NetTargets targets, eMessagePacketType messageType, const char* message, uint32 dataSize = 0);
void SendCallFunctionPacketMulti(NetTargets targets, const VariantVector& data, int32 netID = -1, int32 delay = -1);
void SendCallFunctionPacketMulti(NetTargets targets, uint8* pExtraData, uint32 extraSize, int32 netID = -1,
                                 int32 delay = -1);

inline NetTargets MakeNetTargets(std::vector<uint32>&& targets)
{
    return std::make_shared<const std::vector<uint32>>(std::move(targets));
}

inline NetTargets MakeNetTargets(const std::vector<uint32>& targets)
{
    return std::make_shared<const std::vector<uint32>>(targets);
}

inline void SendUDPPacketRawMulti(std::vector<uint32>&& targets, eMessagePacketType messageType, void* pData,
                                  uint32 dataSize, uint8* pExtraData)
{
    SendUDPPacketRawMulti(MakeNetTargets(std::move(targets)), messageType, pData, dataSize, pExtraData);
}

inline void SendUDPPacketRawMulti(const std::vector<uint32>& targets, eMessagePacketType messageType, void* pData,
                                  uint32 dataSize, uint8* pExtraData)
{
    SendUDPPacketRawMulti(MakeNetTargets(targets), messageType, pData, dataSize, pExtraData);
}

inline void SendUDPPacketMulti(std::vector<uint32>&& targets, eMessagePacketType messageType, const char* message,
                               uint32 dataSize = 0)
{
    SendUDPPacketMulti(MakeNetTargets(std::move(targets)), messageType, message, dataSize);
}

inline void SendUDPPacketMulti(const std::vector<uint32>& targets, eMessagePacketType messageType, const char* message,
                               uint32 dataSize = 0)
{
    SendUDPPacketMulti(MakeNetTargets(targets), messageType, message, dataSize);
}

inline void SendCallFunctionPacketMulti(std::vector<uint32>&& targets, const VariantVector& data, int32 netID = -1,
                                        int32 delay = -1)
{
    SendCallFunctionPacketMulti(MakeNetTargets(std::move(targets)), data, netID, delay);
}

inline void SendCallFunctionPacketMulti(const std::vector<uint32>& targets, const VariantVector& data, int32 netID = -1,
                                        int32 delay = -1)
{
    SendCallFunctionPacketMulti(MakeNetTargets(targets), data, netID, delay);
}

inline void SendCallFunctionPacketMulti(std::vector<uint32>&& targets, uint8* pExtraData, uint32 extraSize,
                                        int32 netID = -1, int32 delay = -1)
{
    SendCallFunctionPacketMulti(MakeNetTargets(std::move(targets)), pExtraData, extraSize, netID, delay);
}

inline void SendCallFunctionPacketMulti(const std::vector<uint32>& targets, uint8* pExtraData, uint32 extraSize,
                                        int32 netID = -1, int32 delay = -1)
{
    SendCallFunctionPacketMulti(MakeNetTargets(targets), pExtraData, extraSize, netID, delay);
}

const char* GetTextFromEnetPacket(uint8* pData, uint32 dataLength);
uint32 GetMessageTypeFromEnetPacket(uint8* pData, uint32 dataLength);
GameUpdatePacket* GetGamePacketFromEnetPacket(uint8* pData, uint32 dataLength, bool checkExtra = true);
uint8* GetExtendedDataFromGamePacket(GameUpdatePacket* pUpdatePacket);