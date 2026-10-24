#include "PlayerInventory.h"
#include "../IO/Log.h"
#include "../Item/ItemInfoManager.h"
#include "../Packet/NetPacket.h"
#include "Player.h"

void InventoryItemInfo::Serialize(MemoryBuffer& memBuffer, bool write, bool database)
{
    if (write)
    {
        int16 idToSend = database ? id : ToItemClientID(id);
        memBuffer.ReadWrite(idToSend, write);
    }
    else
        memBuffer.ReadWrite(id, write);

    memBuffer.ReadWrite(count, write);
    memBuffer.ReadWrite(flags, write);
}

PlayerInventory::PlayerInventory() : m_capacity(INVENTORY_DEFAULT_CAPACITY)
{
    for (uint8 i = 0; i < BODY_PART_SIZE; ++i)
    {
        m_clothes[i] = 0;
    }

    for (uint8 i = 0; i < 4; ++i)
    {
        m_quickSlots[i] = 0;
    }

    m_items.reserve(INVENTORY_DEFAULT_CAPACITY);
    m_quickSlots[0] = ITEM_ID_FIST;
}

void PlayerInventory::Serialize(MemoryBuffer& memBuffer, bool write, bool database)
{
    if (!database)
    {
        memBuffer.ReadWrite(m_version, write);
    }

    bool wideCount = database || m_version != INVENTORY_VERSION_LEGACY;

    uint32 capacity = m_capacity;
    if (!database)
    {
        if (!wideCount && m_capacity > INVENTORY_LEGACY_MAX_ITEMS)
            capacity = INVENTORY_LEGACY_MAX_ITEMS;
    }

    memBuffer.ReadWrite(capacity, write);

    uint16 itemCount = 0;
    if (write)
    {
        itemCount = m_items.size();

        if (!wideCount && itemCount > INVENTORY_LEGACY_MAX_ITEMS)
        {
            itemCount = INVENTORY_LEGACY_MAX_ITEMS;
        }
    }

    if (wideCount)
    {
        memBuffer.ReadWrite(itemCount, write);
    }
    else
    {
        uint8 narrowCount = (uint8)itemCount;
        memBuffer.ReadWrite(narrowCount, write);
        itemCount = narrowCount;
    }

    if (write)
    {
        for (uint16 i = 0; i < itemCount; ++i)
        {
            m_items[i].Serialize(memBuffer, true, database);
        }
        return;
    }

    m_items.clear();
    m_items.reserve(m_capacity);

    for (uint8 i = 0; i < BODY_PART_SIZE; ++i)
    {
        m_clothes[i] = 0;
    }

    ItemInfoManager* pItemMgr = GetItemInfoManager();
    for (uint16 i = 0; i < itemCount; ++i)
    {
        InventoryItemInfo item;
        item.Serialize(memBuffer, false, database);

        if (item.id == 0 || item.count == 0)
            continue;

        if (IsIllegalItem(item.id))
        {
            LOGGER_LOG_WARN("Illegal item %d found in player inventory skipping adding", item.id);
            continue;
        }

        ItemInfo* pItem = pItemMgr->GetItemByID(item.id);
        if (!pItem)
            continue;

        if (item.flags & INVENTORY_FLAG_WEARING)
        {
            uint8 bodyPart = (uint8)pItem->GetBodyPart();
            if (bodyPart < BODY_PART_SIZE)
            {
                m_clothes[bodyPart] = item.id;
            }
        }

        m_items.push_back(std::move(item));
    }
}

InventoryItemInfo* PlayerInventory::GetItemByID(int32 itemID)
{
    for (auto& item : m_items)
    {
        if (item.id == itemID)
            return &item;
    }

    return nullptr;
}

int32 PlayerInventory::GetItemIndex(int32 itemID) const
{
    for (int32 i = 0; i < m_items.size(); ++i)
    {
        if (m_items[i].id == itemID)
            return i;
    }

    return -1;
}

bool PlayerInventory::IsPinned(const InventoryItemInfo& item) const
{
    if (item.flags & INVENTORY_FLAG_WEARING)
        return true;

    if (item.id == ITEM_ID_FIST || item.id == ITEM_ID_WRENCH)
        return true;

    for (uint8 i = 0; i < 4; ++i)
    {
        if (m_quickSlots[i] == item.id)
            return true;
    }

    return false;
}

uint8 PlayerInventory::AddItem(int32 itemID, uint8 count, Player* pPlayer)
{
    if (itemID == ITEM_ID_GEMS)
        return 0;

    ItemInfo* pItemInfo = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItemInfo)
        return 0;

    InventoryItemInfo* pSearchItem = GetItemByID(itemID);
    if (!pSearchItem)
    {
        if (m_items.size() + 1 > m_capacity)
            return 0;

        if (pItemInfo->IsUnlimited())
        {
            count = 1;
        }
        else if (count > pItemInfo->maxCanHold)
        {
            count = pItemInfo->maxCanHold;
        }

        InventoryItemInfo item;
        item.id = itemID;
        item.count = count;
        item.flags = 0;

        m_items.push_back(std::move(item));

        if (IsClientVisible(m_items.size() - 1))
        {
            UpdateInventory(pPlayer, itemID, count, true);
        }
        return count;
    }

    if (pSearchItem->count + count > pItemInfo->maxCanHold)
        return 0;

    int32 index = (pSearchItem - m_items.data());
    pSearchItem->count += count;

    if (IsClientVisible(index))
    {
        UpdateInventory(pPlayer, itemID, count, true);
    }
    return count;
}

uint8 PlayerInventory::RemoveItem(int32 itemID, int16 count, Player* pPlayer)
{
    if (count <= 0)
        return 0;

    InventoryItemInfo* pItem = GetItemByID(itemID);
    if (!pItem)
        return 0;

    if (count >= pItem->count)
    {
        count = pItem->count;
    }

    if (count == pItem->count)
        return RemoveItem(itemID, pPlayer);

    int32 index = (pItem - m_items.data());
    pItem->count -= count;

    if (IsClientVisible(index))
    {
        UpdateInventory(pPlayer, itemID, count, false);
    }
    return count;
}

uint8 PlayerInventory::RemoveItem(int32 itemID, Player* pPlayer)
{
    int32 index = GetItemIndex(itemID);
    if (index < 0)
    {
        return 0;
    }

    uint8 itemCount = m_items[index].count;
    bool wasVisible = IsClientVisible(index);

    m_items.erase(m_items.begin() + index);

    if (wasVisible)
    {
        UpdateInventory(pPlayer, itemID, itemCount, false);

        if (IsLegacy() && m_items.size() >= INVENTORY_LEGACY_MAX_ITEMS)
        {
            InventoryItemInfo& promoted = m_items[INVENTORY_LEGACY_MAX_ITEMS - 1];
            UpdateInventory(pPlayer, promoted.id, promoted.count, true);
        }
    }

    RemoveFromQuickSlots(itemID);
    return itemCount;
}

void PlayerInventory::SetClothByPart(int32 itemID, uint8 bodyPart, Player* pPlayer)
{
    if (bodyPart >= BODY_PART_SIZE)
        return;

    if (itemID != ITEM_ID_BLANK && IsLegacy())
    {
        EnsureVisible(itemID, pPlayer);
    }

    if (InventoryItemInfo* pOldItem = GetItemByID(m_clothes[bodyPart]))
    {
        if (itemID != ITEM_ID_BLANK)
        {
            pOldItem->flags &= ~INVENTORY_FLAG_WEARING;
        }
    }

    if (InventoryItemInfo* pNewItem = GetItemByID(itemID))
    {
        if (itemID != ITEM_ID_BLANK)
        {
            pNewItem->flags |= INVENTORY_FLAG_WEARING;
        }
    }

    m_clothes[bodyPart] = itemID;
}

bool PlayerInventory::IsWearingItem(int32 itemID)
{
    for (uint8 i = 0; i < BODY_PART_SIZE; ++i)
    {
        if (m_clothes[i] == itemID)
            return true;
    }

    return false;
}

bool PlayerInventory::HaveRoomForItem(int32 itemID, uint8 itemCount)
{
    InventoryItemInfo* pItem = GetItemByID(itemID);
    ItemInfo* pItemInfo = GetItemInfoManager()->GetItemByID(itemID);

    if (!pItemInfo)
        return false;

    if (pItem)
        return (pItem->count + itemCount <= pItemInfo->maxCanHold);

    if (itemCount > pItemInfo->maxCanHold || m_items.size() + 1 > m_capacity)
        return false;

    return true;
}

uint32 PlayerInventory::GetFitItemCount(int32 itemID)
{
    if (itemID == ITEM_ID_GEMS || itemID == ITEM_ID_CARRIED_GAME_FLAG || itemID == ITEM_ID_CARRIED_GAME_EGG)
        return 999999;

    InventoryItemInfo* pItem = GetItemByID(itemID);
    ItemInfo* pInfo = GetItemInfoManager()->GetItemByID(itemID);

    if (!pInfo)
        return 0;

    if (!pItem)
        return m_items.size() < m_capacity ? pInfo->maxCanHold : 0;

    if (pItem->count >= pInfo->maxCanHold)
        return 0;

    return pInfo->maxCanHold - pItem->count;
}

// count, itemID, count, itemID
bool PlayerInventory::CanAllItemsFit(const std::vector<int32>& items)
{
    ItemInfoManager* pItemMgr = GetItemInfoManager();
    if (items.size() % 2 != 0)
        return false;

    for (uint16 i = 0; i < items.size(); i += 2)
    {
        int32 itemID = items[i + 1];
        int32 count = items[i];

        ItemInfo* pItem = pItemMgr->GetItemByID(itemID);
        if (!pItem)
            return false;

        if (count > pItem->maxCanHold)
            return false;

        InventoryItemInfo* pInvItem = GetItemByID(itemID);
        if (pInvItem && pInvItem->count + count > pItem->maxCanHold)
            return false;

        if (!pInvItem && m_items.size() + 1 > m_capacity)
            return false;
    }

    return true;
}

uint8 PlayerInventory::GetCountOfItem(int32 itemID)
{
    InventoryItemInfo* pItem = GetItemByID(itemID);
    if (!pItem)
        return 0;

    return pItem->count;
}

void PlayerInventory::UpdateInventory(Player* pPlayer, int32 itemID, uint8 count, bool added)
{
    if (!pPlayer)
        return;

    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_MODIFY_ITEM_INVENTORY;
    packet.field_7 = ToItemClientID(itemID);

    if (added)
    {
        packet.field_3 = count;
    }
    else
    {
        packet.field_2 = count;
    }

    SendUDPPacketRaw(pPlayer->GetNetID(), NET_MESSAGE_GAME_PACKET, &packet, sizeof(GameUpdatePacket), nullptr);
}

void PlayerInventory::RemoveFromQuickSlots(int32 itemID)
{
    for (uint8 i = 0; i < 4; ++i)
    {
        if (m_quickSlots[i] == itemID)
        {
            m_quickSlots[i] = 0;
        }
    }
}

bool PlayerInventory::IsWearingPlayMod(int32 playModType)
{
    if (playModType == 0)
        return false;

    ItemInfoManager* pItemMgr = GetItemInfoManager();

    for (auto& item : m_items)
    {
        if (item.id == ITEM_ID_BLANK)
            continue;

        ItemInfo* pItem = pItemMgr->GetItemByID(item.id);
        if (pItem && pItem->playModType == playModType)
            return true;
    }

    return false;
}

uint32 PlayerInventory::GetMemEstimate(bool database)
{
    int32 itemCount = m_items.size();
    if (!database && IsLegacy())
    {
        itemCount = Min(itemCount, INVENTORY_LEGACY_MAX_ITEMS);
    }

    uint32 memEstimate = sizeof(m_capacity) + itemCount * sizeof(InventoryItemInfo);

    if (!database)
    {
        memEstimate += sizeof(m_version);
    }

    if (database || m_version != INVENTORY_VERSION_LEGACY)
        memEstimate += sizeof(uint16);
    else
        memEstimate += sizeof(uint8);

    return memEstimate;
}

void PlayerInventory::SetVersion(uint32 protocol)
{
    m_version = protocol >= INVENTORY_PROTOCOL_BIG_BACKPACK ? INVENTORY_VERSION_BIG : INVENTORY_VERSION_LEGACY;
}

uint32 PlayerInventory::GetOverflowCount() const
{
    if (!IsLegacy() || m_items.size() <= INVENTORY_LEGACY_MAX_ITEMS)
        return 0;

    return (uint32)(m_items.size() - INVENTORY_LEGACY_MAX_ITEMS);
}

InventoryItemInfo* PlayerInventory::GetOverflowItem(uint32 overflowIndex)
{
    if (overflowIndex >= GetOverflowCount())
        return nullptr;

    return &m_items[INVENTORY_LEGACY_MAX_ITEMS + overflowIndex];
}

void PlayerInventory::SwapVisibleWithHidden(int32 visibleIdx, int32 hiddenIdx, Player* pPlayer)
{
    int16 outID = m_items[visibleIdx].id;
    uint8 outCount = m_items[visibleIdx].count;
    int16 inID = m_items[hiddenIdx].id;
    uint8 inCount = m_items[hiddenIdx].count;

    std::swap(m_items[visibleIdx], m_items[hiddenIdx]);

    UpdateInventory(pPlayer, outID, outCount, false);
    UpdateInventory(pPlayer, inID, inCount, true);
}

eInventorySwapResult PlayerInventory::SwapWithOverflow(int32 visibleItemID, int32 hiddenItemID, Player* pPlayer)
{
    if (!IsLegacy())
        return INVENTORY_SWAP_NOT_LEGACY;

    int32 hiddenIdx = GetItemIndex(hiddenItemID);
    if (hiddenIdx < 0 || hiddenIdx < INVENTORY_LEGACY_MAX_ITEMS)
        return INVENTORY_SWAP_HIDDEN_NOT_FOUND;

    int32 visibleIdx = GetItemIndex(visibleItemID);
    if (visibleIdx < 0 || visibleIdx >= INVENTORY_LEGACY_MAX_ITEMS)
        return INVENTORY_SWAP_VISIBLE_NOT_FOUND;

    if (IsPinned(m_items[visibleIdx]))
        return INVENTORY_SWAP_VISIBLE_PINNED;

    SwapVisibleWithHidden(visibleIdx, hiddenIdx, pPlayer);
    return INVENTORY_SWAP_OK;
}

void PlayerInventory::EnsureVisible(int32 itemID, Player* pPlayer)
{
    int32 index = GetItemIndex(itemID);
    if (index < INVENTORY_LEGACY_MAX_ITEMS)
        return;

    for (int32 i = INVENTORY_LEGACY_MAX_ITEMS - 1; i >= 0; --i)
    {
        if (!IsPinned(m_items[i]))
        {
            SwapVisibleWithHidden(i, index, pPlayer);
            return;
        }
    }
}