#pragma once

#include "../Precompiled.h"
#include "GamePacket.h"
#include <enet/enet.h>

bool SendENetPacketRaw(eMessagePacketType messageType, void* pData, uint32 dataSize, uint8* pExtraData, ENetPeer* pPeer);
bool SendENetPacket(eMessagePacketType messageType, const char* message, ENetPeer* pPeer);
const char* GetTextFromEnetPacket(ENetPacket* pPacket);
uint32 GetMessageTypeFromEnetPacket(ENetPacket* pPacket);
GameUpdatePacket* GetGamePacketFromEnetPacket(ENetPacket* pPacket);
uint8* GetExtendedDataFromGamePacket(GameUpdatePacket* pUpdatePacket);