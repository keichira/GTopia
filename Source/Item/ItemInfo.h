#pragma once

#include "../Precompiled.h"
#include "../Math/Color.h"
#include "../Math/Vector2.h"
#include "../Memory/MemoryBuffer.h"
#include "ItemUtils.h"

class ItemInfo {
public:
    ItemInfo();

public:
    uint32 id = 0;
    uint16 flags = 0;
    uint8 type = 0;
    
    union {
        uint8 material = 0;
        uint8 clothSleeve;
    };

    string name = "";
    string textureFile = "";
    uint32 textureHash = 0;
    uint8 visualEffect = 0;
    int32 cookingTime = -1;
    uint8 textureX = 0;
    uint8 textureY = 0;
    uint8 storage = 0;

    int8 layer = 0;
    uint8 collisionType = 0;
    uint8 hp = 0;
    int32 restoreTime = 0;
    uint8 bodyPart = 0;
    int16 rarity = 1;
    uint8 maxCanHold = 200;

    string extraString = "";
    uint32 extraStringHash = 0;

    int32 animMS = 0;
    
    string petName = "";
    string petSubName = "";
    string petEndName = "";
    string petPowerName = "";

    uint8 seedBg = 0;
    uint8 seedFg = 0;
    uint8 treeBg = 0;
    uint8 treeFg = 0;
    Color seedBgColor;
    Color seedFgColor;

    uint16 seed1 = 0;
    uint16 seed2 = 0;

    uint32 growTime = 31;

    uint32 fxFlags = 0;
    string multiAnim1;
    string overlayTextureFile;
    string multiAnim2;

    Vector2Int dualAnimLayer;
    uint32 flags2;

    string description = "No info.";
    uint8 element = ITEM_ELEMENT_NONE;

public:
    bool HasFlag(uint16 flag) { return (flags & flag) != 0; }
    void Serialize(MemoryBuffer& memBuffer, bool write, uint16 version);

    bool IsBackground();
};

bool IsIllegalItem(uint32 itemID);