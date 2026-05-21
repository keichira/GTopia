#pragma once

#include "../Precompiled.h"
#include "ItemInfo.h"

#define MAX_SUPPORTED_ITEM_DATA_VERSION 26

static const std::vector<std::pair<float, uint16>> sItemDataVersionMap = {
    {5.47f, 26},
    {5.46f, 25},
    {5.11f, 21},
    {4.71f, 19},
    {4.53f, 17},
    {4.44f, 16},
    {4.19f, 15},
    {3.74f, 14},
    {3.62f, 13},
    {3.45f, 12},
    {2.988f, 11}
};

uint16 GetSupportedItemDataVersion(float gameVersion);
uint16 GetMinRequiredItemDataVersion(float a, float b, float c, float d);
uint16 GetMaxRequiredItemDataVersion(float a, float b, float c, float d);

struct ItemsClientData
{
    uint8* pItemData = nullptr;
    uint32 size = 0;
    uint32 compressSize = 0;
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
    bool LoadWikiData(const string& filePath);

    void Kill();

    void LoadFileHashes(const std::unordered_map<string, uint32>& hashData, bool forOgg);
    void SaveToClientData(bool forOgg, uint16 minVersion, uint16 maxVersion);
    
    uint32 GetBaseItemID(uint32 itemID);
    ItemInfo* GetItemByID(uint32 itemID);
    ItemInfo* GetItemByName(const string& name);
    uint32 GetItemCount() const { return m_itemCount; }
    void ForceItemDataVersion(uint16 newVersion) { m_version = newVersion; }

    ItemInfo* GetSpliceInfo(uint16 seed1, uint16 seed2);
    void SetupItemExtras();

    ItemsClientData* GetClientData(uint8 platformType, float gameVersion);

private:
    uint32 GetItemRarity(uint32 itemID);
    void CreateDefaultSeedForItem(ItemInfo* pItem);

private:
    uint16 m_version;
    uint32 m_itemCount;

    ItemsClientData m_itemDataMp3[MAX_SUPPORTED_ITEM_DATA_VERSION];
    ItemsClientData m_itemDataOgg[MAX_SUPPORTED_ITEM_DATA_VERSION];

    std::vector<ItemInfo> m_items;
    std::unordered_map<uint32, uint32> m_spliceData;
};

ItemInfoManager* GetItemInfoManager();