#pragma once

#include "../Precompiled.h"
#include "ItemInfo.h"

#define ITEM_DATA_VERSION 1

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
    bool LoadFileHashes(const std::vector<string>& fileData);
    ItemInfo* GetItemByID(uint32 itemID);

private:
    void SetupItemExtras();
    void CreateDefaultSeedForItem(ItemInfo* pItem);

private:
    uint16 m_version;
    uint32 m_itemCount;

    std::vector<ItemInfo*> m_items;
};

ItemInfoManager* GetItemInfoManager();