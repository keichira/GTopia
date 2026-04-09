#include "PlayerInventory.h"
#include "../Item/ItemInfoManager.h"
#include "../IO/Log.h"
#include "Player.h"
#include "../Packet/NetPacket.h"

void InventoryItemInfo::Serialize(MemoryBuffer& memBuffer, bool write)
{
    memBuffer.ReadWrite(id, write);
    memBuffer.ReadWrite(count, write);
    memBuffer.ReadWrite(flags, write);
}

PlayerInventory::PlayerInventory()
: m_capacity(INVENTORY_DEFAULT_CAPACITY)
{
    for(uint8 i = 0; i < BODY_PART_SIZE; ++i) {
        m_clothes[i] = 0;
    }

    m_items.reserve(INVENTORY_DEFAULT_CAPACITY);

    //m_items.emplace_back(InventoryItemInfo{ ITEM_ID_FIST, 1, 0 });
    //m_items.emplace_back(InventoryItemInfo{ ITEM_ID_WRENCH, 1, 0 });

    m_quickSlots[0] = ITEM_ID_FIST;
}

void PlayerInventory::Serialize(MemoryBuffer& memBuffer, bool write, bool database)
{
    if(!database) {
        memBuffer.ReadWrite(m_version, write);
    }

    memBuffer.ReadWrite(m_capacity, write);

    uint16 invItemSize = m_items.size();
    if(database || m_version > 0) {
        memBuffer.ReadWrite(invItemSize, write);
    }
    else {
        uint8 tempInvSize = (uint8)invItemSize;
        memBuffer.ReadWrite(tempInvSize, write);

        if(!write) {
            invItemSize = tempInvSize;
        }
    }

    if(!write) {
        m_items.reserve(m_capacity);
    }

    if(m_version == 0 && !database && invItemSize > 255) {
        invItemSize = 250;
    }

    ItemInfoManager* pItemMgr = GetItemInfoManager();

    for(uint16 i = 0; i < invItemSize; ++i) {
        if(write) {
            m_items[i].Serialize(memBuffer, true);
        }
        else {
            InventoryItemInfo item;
            item.Serialize(memBuffer, write);
    
            if(IsIllegalItem(item.id) || item.id < 0) {
                LOGGER_LOG_WARN("Illegal item %d found in player inventory skipping adding", item.id);
                continue;
            }

            ItemInfo* pItem = pItemMgr->GetItemByID(item.id);
            if(!pItem) {
                continue;
            }

            if(item.flags == 1) {
                m_clothes[pItem->bodyPart] = item.id;
            }
    
            m_items.push_back(std::move(item));
        }
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

uint8 PlayerInventory::AddItem(uint16 itemID, uint8 count, Player* pPlayer)
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

        if(itemID == ITEM_ID_FIST || itemID == ITEM_ID_WRENCH) {
            count = 1;
        }
        else if(count > pItemInfo->maxCanHold) {
            count = pItemInfo->maxCanHold;
        }

        InventoryItemInfo item;
        item.id = itemID;
        item.count = count;
        item.flags = 0;

        m_items.push_back(std::move(item));
        UpdateInventory(pPlayer, itemID, count, true);
        return count;
    }

    if(pSearchItem->count + count > pItemInfo->maxCanHold) {
        return 0;
    }

    pSearchItem->count += count;
    UpdateInventory(pPlayer, itemID, count, true);
    return count;
}

uint8 PlayerInventory::RemoveItem(uint16 itemID, int16 count, Player* pPlayer)
{
    InventoryItemInfo* pItem = GetItemByID(itemID);
    if(!pItem) {
        return 0;
    }

    if(count >= pItem->count) {
        count = pItem->count;
    }

    if(count == pItem->count) {
        return RemoveItem(itemID, pPlayer);
    }

    pItem->count -= count;
    UpdateInventory(pPlayer, itemID, count, false);
    return count;
}

uint8 PlayerInventory::RemoveItem(uint16 itemID, Player* pPlayer)
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

    UpdateInventory(pPlayer, itemID, itemCount, false);
    RemoveFromQuickSlots(itemID);
    return itemCount;
}

void PlayerInventory::SetClothByPart(uint16 itemID, uint8 bodyPart)
{
    if(bodyPart > BODY_PART_SIZE) {
        return;
    }

    InventoryItemInfo* pItem = GetItemByID(itemID);
    if(pItem) {
        if(itemID == ITEM_ID_BLANK) {
            pItem->flags = 0;
        }
        else {
            pItem->flags = 1;
        }
    }

    m_clothes[bodyPart] = itemID;
}

bool PlayerInventory::IsWearingItem(uint16 itemID)
{
    for(uint8 i = 0; i < BODY_PART_SIZE; ++i) {
        if(m_clothes[i] == itemID) {
            return true;
        }
    }

    return false;
}

bool PlayerInventory::HaveRoomForItem(uint16 itemID, uint8 itemCount)
{
    InventoryItemInfo* pItem = GetItemByID(itemID);
    ItemInfo* pItemInfo = GetItemInfoManager()->GetItemByID(itemID);
    
    if(!pItemInfo) {
        return false;
    }

    if(pItem) {
        return (pItem->count + itemCount <= pItemInfo->maxCanHold);
    }

    if(
        itemCount > pItemInfo->maxCanHold ||
        m_items.size() + 1 > m_capacity
    ) {
        return false;
    }

    return true;
}

uint8 PlayerInventory::GetCountOfItem(uint16 itemID)
{
    InventoryItemInfo* pItem = GetItemByID(itemID);
    if(!pItem) {
        return 0;
    }

    return pItem->count;
}

void PlayerInventory::UpdateInventory(Player *pPlayer, uint16 itemID, uint8 count, bool added)
{
    if(!pPlayer) {
        return;
    }

    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_MODIFY_ITEM_INVENTORY;
    packet.itemID = itemID;

    if(added) {
        packet.addedItemCount = count;
    }
    else {
        packet.removedItemCount = count;
    }

    SendENetPacketRaw(NET_MESSAGE_GAME_PACKET, &packet, sizeof(GameUpdatePacket), nullptr, pPlayer->GetPeer());
}

void PlayerInventory::RemoveFromQuickSlots(uint16 itemID)
{
    for(uint8 i = 0; i < 4; ++i) {
        if(m_quickSlots[i] == itemID) {
            m_quickSlots[i] = 0;
        }
    }
}

uint32 PlayerInventory::GetMemEstimate(bool database)
{
    uint32 memEstimate = sizeof(m_capacity) + m_items.size() * sizeof(InventoryItemInfo);

    if(!database) {
        memEstimate += sizeof(m_version);
    }

    if(database || m_version > 0) {
        memEstimate += sizeof(uint16);
    }
    else {
        memEstimate += sizeof(uint8);
    }

    return memEstimate;
}

void PlayerInventory::SetVersion(uint32 protocol)
{
    /*if(protocol < 94) {
        m_version = 0;
    }
    else {
        m_version = 1;
    }*/

    m_version = 0;
}
