#include "PlayerInventory.h"
#include "../Item/ItemInfoManager.h"
#include "../IO/Log.h"

void InventoryItemInfo::Serialize(MemoryBuffer& memBuffer, bool write)
{
    memBuffer.ReadWrite(id, write);
    memBuffer.ReadWrite(count, write);
    memBuffer.ReadWrite(flags, write);
}

PlayerInventory::PlayerInventory(uint32 protocol)
: m_capacity(INVENTORY_DEFAULT_CAPACITY)
{
    if(protocol < 93) {
        m_version = 0;
    }
    else {
        m_version = 1;
    }

    for(auto i = 0; i < BODY_PART_SIZE; ++i) {
        m_clothes[i] = 0;
    }

    m_items.reserve(INVENTORY_DEFAULT_CAPACITY);

    m_items.emplace_back(InventoryItemInfo{ ITEM_ID_FIST, 1, 0 });
    m_items.emplace_back(InventoryItemInfo{ ITEM_ID_WRENCH, 1, 0 });
}

void PlayerInventory::Serialize(MemoryBuffer& memBuffer, bool write, uint32 protocol, bool database)
{
    memBuffer.ReadWrite(m_version, write);
    memBuffer.ReadWrite(m_capacity, write);

    uint16 invItemSize = m_items.size();
    if(database) {
        memBuffer.ReadWrite(invItemSize, write);
    }
    else {
        if(protocol < 94) {
            uint8 tempInvSize = (uint8)invItemSize;
            memBuffer.ReadWrite(tempInvSize, write);
        }
        else {
            memBuffer.ReadWrite(invItemSize, write);
        }
    }

    if(!write) {
        m_items.reserve(m_capacity);
    }

    if(protocol < 94 && !database && invItemSize > 255) {
        invItemSize = 250;
    }

    for(uint16 i = 0; i < invItemSize; ++i) {
        InventoryItemInfo item;
        item.Serialize(memBuffer, write);

        if(!write && (IsIllegalItem(item.id)) || item.id < 0) {
            LOGGER_LOG_WARN("Illegal item %d found in player inventory skipping adding", item.id);
            continue;
        }

        m_items.push_back(std::move(item));
    }
}

InventoryItemInfo* PlayerInventory::GetItemByID(uint16 itemID)
{
    for(auto& item : m_items) {
        if(item.id == itemID) {
            return &item;
        }
    }

    return nullptr;
}

uint8 PlayerInventory::AddItem(uint16 itemID, uint8 count, eInventoryErrors& errorCode)
{
    ItemInfo* pItemInfo = GetItemInfoManager()->GetItemByID(itemID);
    if(!pItemInfo) {
        return 0;
    }

    InventoryItemInfo* pSearchItem = GetItemByID(itemID);
    if(!pSearchItem) {
        if(m_items.size() + 1 > m_capacity) {
            return 0;
        }

        if(count > pItemInfo->maxCanHold) {
            count = pItemInfo->maxCanHold;
        }

        InventoryItemInfo item;
        item.id = itemID;
        item.count = count;
        item.flags = 0;

        m_items.push_back(std::move(item));
        return count;
    }

    if(pSearchItem->count + count > pItemInfo->maxCanHold) {
        return 0;
    }

    pSearchItem->count += count;
    return count;
}

uint8 PlayerInventory::RemoveItem(uint16 itemID, uint8 count)
{
    InventoryItemInfo* pItem = GetItemByID(itemID);
    if(!pItem) {
        return 0;
    }

    if(pItem->count - count <= 0) {
        return RemoveItem(itemID);
    }

    pItem->count -= count;
    return count;
}

uint8 PlayerInventory::RemoveItem(uint16 itemID)
{
    InventoryItemInfo* pItem = GetItemByID(itemID);
    if(!pItem) {
        return 0;
    }

    uint8 itemCount = pItem->count;
    if(pItem != &m_items.back()) {
        *pItem = std::move(m_items.back());
    }
    m_items.pop_back();

    RemoveFromQuickSlots(itemID);
    return itemCount;
}

void PlayerInventory::RemoveFromQuickSlots(uint16 itemID)
{
    for(uint8 i = 0; i < 4; ++i) {
        if(m_quickSlots[i] == itemID) {
            m_quickSlots[i] = 0;
        }
    }
}

uint32 PlayerInventory::GetMemEstimate(bool database, uint32 protocol)
{
    uint32 memEstimate = m_items.size() * sizeof(InventoryItemInfo);

    if(!database && protocol < 94) {
        memEstimate += sizeof(uint8);
    }
    else {
        memEstimate += sizeof(uint16);
    }

    return memEstimate;
}
