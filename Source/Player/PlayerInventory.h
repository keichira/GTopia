#pragma once

#include "../Precompiled.h"
#include "../Item/ItemUtils.h"
#include "../Memory/MemoryBuffer.h"

#define INVENTORY_DEFAULT_CAPACITY 16

struct InventoryItemInfo
{
    uint16 id = 0;
    uint8 count = 0;
    uint8 flags = 0;

    void Serialize(MemoryBuffer& memBuffer, bool write);
};

enum eInventoryErrors
{
    INVENTORY_ERROR_UNKNOWN,
    INVENTORY_ERROR_CAPACITY,
    INVENTORY_ERROR_MAX_HOLD
};

class Player;

class PlayerInventory {
public:
    PlayerInventory();

public:
    void Serialize(MemoryBuffer& memBuffer, bool write, bool database);
    InventoryItemInfo* GetItemByID(uint16 itemID);

    uint8 AddItem(uint16 itemID, uint8 count, Player* pPlayer = nullptr);
    uint8 RemoveItem(uint16 itemID, int16 count, Player* pPlayer = nullptr);
    uint8 RemoveItem(uint16 itemID, Player* pPlayer = nullptr);

    const uint16* GetClothes() const { return m_clothes; }
    uint16 GetClothByPart(eBodyPart bodyPart) const { return m_clothes[bodyPart]; }
    void SetClothByPart(uint16 itemID, uint8 bodyPart);
    bool IsWearingItem(uint16 itemID);

    bool HaveRoomForItem(uint16 itemID, uint8 itemCount);
    uint32 GetFitItemCount(uint16 itemID);
    bool CanAllItemsFit(const std::vector<uint32>& items);

    uint8 GetCountOfItem(uint16 itemID);

    void UpdateInventory(Player* pPlayer, uint16 itemID, uint8 count, bool added);
    void RemoveFromQuickSlots(uint16 itemID);

    uint32 GetMemEstimate(bool database);
    void SetVersion(uint32 protocol);

    uint32 GetInventorySize() const { return m_capacity; }

private:
    uint8 m_version;
    uint32 m_capacity;

    std::vector<InventoryItemInfo> m_items;
    uint16 m_clothes[BODY_PART_SIZE];
    uint16 m_quickSlots[4];
};