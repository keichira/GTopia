#pragma once

#include "../Precompiled.h"
#include "../Item/ItemUtils.h"
#include "../Memory/MemoryBuffer.h"

#define INVENTORY_DEFAULT_CAPACITY 16

struct InventoryItemInfo
{
    int16 id;
    uint8 count;
    uint8 flags;

    void Serialize(MemoryBuffer& memBuffer, bool write);
};

enum eInventoryErrors
{
    INVENTORY_ERROR_UNKNOWN,
    INVENTORY_ERROR_CAPACITY,
    INVENTORY_ERROR_MAX_HOLD
};

class PlayerInventory {
public:
    PlayerInventory(uint32 protocol);

public:
    void Serialize(MemoryBuffer& memBuffer, bool write, uint32 protocol, bool database);
    InventoryItemInfo* GetItemByID(uint16 itemID);

    uint8 AddItem(uint16 itemID, uint8 count, eInventoryErrors& errorCode);
    uint8 RemoveItem(uint16 itemID, uint8 count);
    uint8 RemoveItem(uint16 itemID);

    void RemoveFromQuickSlots(uint16 itemID);

    uint32 GetMemEstimate(bool database, uint32 protocol = 0);

private:
    uint8 m_version;
    uint32 m_capacity;

    std::vector<InventoryItemInfo> m_items;
    uint16 m_clothes[BODY_PART_SIZE];
    uint16 m_quickSlots[4];
};