#pragma once

#include "../Item/ItemUtils.h"
#include "../Memory/MemoryBuffer.h"
#include "../Precompiled.h"

#define INVENTORY_DEFAULT_CAPACITY 16

struct InventoryItemInfo
{
    int16 id = 0;
    uint8 count = 0;
    uint8 flags = 0;

    void Serialize(MemoryBuffer& memBuffer, bool write, bool database);
};

enum eInventoryErrors
{
    INVENTORY_ERROR_UNKNOWN,
    INVENTORY_ERROR_CAPACITY,
    INVENTORY_ERROR_MAX_HOLD
};

class Player;

class PlayerInventory
{
public:
    PlayerInventory();

public:
    void Serialize(MemoryBuffer& memBuffer, bool write, bool database);
    InventoryItemInfo* GetItemByID(int32 itemID);

    uint8 AddItem(int32 itemID, uint8 count, Player* pPlayer = nullptr);
    uint8 RemoveItem(int32 itemID, int16 count, Player* pPlayer = nullptr);
    uint8 RemoveItem(int32 itemID, Player* pPlayer = nullptr);

    const int16* GetClothes() const { return m_clothes; }
    int16 GetClothByPart(eBodyPart bodyPart) const { return m_clothes[bodyPart]; }
    void SetClothByPart(int32 itemID, uint8 bodyPart);
    bool IsWearingItem(int32 itemID);

    bool HaveRoomForItem(int32 itemID, uint8 itemCount);
    uint32 GetFitItemCount(int32 itemID);
    bool CanAllItemsFit(const std::vector<int32>& items);

    uint8 GetCountOfItem(int32 itemID);

    void UpdateInventory(Player* pPlayer, int32 itemID, uint8 count, bool added);
    void RemoveFromQuickSlots(int32 itemID);

    bool IsWearingPlayMod(int32 playModType);

    uint32 GetMemEstimate(bool database);
    void SetVersion(uint32 protocol);

    uint32 GetInventorySize() const { return m_capacity; }
    uint32 GetItemsSize() const { return m_items.size(); }
    uint32 GetInventorySpace() const { return (m_capacity - m_items.size()) < 0 ? 0 : (m_capacity - m_items.size()); }

private:
    uint8 m_version;
    uint32 m_capacity;

    std::vector<InventoryItemInfo> m_items;
    int16 m_clothes[BODY_PART_SIZE];
    int16 m_quickSlots[4];
};