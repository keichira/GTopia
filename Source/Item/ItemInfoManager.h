#pragma once

#include "../Precompiled.h"
#include "ItemInfo.h"

#define ITEM_DATA_VERSION 1

struct ItemsClientData
{
    uint8* pItemData = nullptr;
    uint32 size = 0;
    uint32 hash = 0;
};

class ItemInfoManager {
public:
    ItemInfoManager();
    ~ItemInfoManager();

public:
    static ItemInfoManager* GetInstance() 
    {
        static ItemInfoManager instance;
        return &instance;
    }

public:
    bool Load(const string& filePath);
    bool LoadByItemsDat(const string& filePath);

    void LoadFileHashes(const std::vector<string>& fileData, bool forOgg);
    void LoadItemsClientData(bool forOgg);
    
    ItemInfo* GetItemByID(uint32 itemID);
    ItemInfo* GetItemByName(const string& name);
    uint32 GetItemCount() const { return m_itemCount; }

    ItemsClientData& GetClientData(uint8 platformType);

private:
    void SetupItemExtras();
    void CreateDefaultSeedForItem(ItemInfo* pItem);

private:
    uint16 m_version;
    uint32 m_itemCount;

    ItemsClientData m_itemDataMp3;
    ItemsClientData m_itemDataOgg;

    std::vector<ItemInfo*> m_items;
};

ItemInfoManager* GetItemInfoManager();