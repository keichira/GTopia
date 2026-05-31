#pragma once

#include "../Precompiled.h"
#include "ItemInfo.h"
#include "BattlePetInfo.h"

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

enum eConsumableType
{
    CONSUMABLE_TYPE_NONE = 0,
    CONSUMABLE_TYPE_CRAFT = 1,

    CONUMABLE_TYPE_SIZE = 2
};

enum eConsumableFlags
{
    CONSUMABLE_FLAG_NEED_TARGET = 1 << 0,
    CONSUMABLE_FLAG_SELF_ONLY = 1 << 1,
    CONSUMABLE_FLAG_NEED_TILE = 1 << 2,
    CONSUMABLE_FLAG_EQUIP = 1 << 3
};

struct ConsumableInfo
{
    int32 itemID = 0;
    uint8 consumableType = 0;
    uint8 requiredAmount = 0;
    int32 rewardItemID = 0;
    uint8 rewardCount = 0;
    uint32 flags = 0;
    string successMessage;
    string failMessage;

    bool HasFlag(uint32 flag) { return flags & flag; };
};

uint32 StrToConsumableFlag(const string& str);

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
    bool LoadConsumableData(const string& filePath);
    bool LoadBattlePetData(const string& filePath);

    void Kill();

    void LoadFileHashes(const std::unordered_map<string, uint32>& hashData, bool forOgg);
    void SaveToClientData(bool forOgg, uint16 minVersion, uint16 maxVersion);
    
    uint32 GetBaseItemID(uint32 itemID);
    ItemInfo* GetItemByID(uint32 itemID);
    ItemInfo* GetItemByName(const string& name);
    uint32 GetItemCount() const { return m_itemCount; }
    void ForceItemDataVersion(uint16 newVersion) { m_version = newVersion; }

    ItemInfo* GetSpliceInfo(uint16 seed1, uint16 seed2);
    ConsumableInfo* GetConsumableInfo(uint32 itemID);
    BattlePetInfo* GetBattlePetInfo(int32 itemID);
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
    std::unordered_map<uint32, ConsumableInfo> m_consumeData;
    std::unordered_map<int32, BattlePetInfo> m_battlePetData;
};

ItemInfoManager* GetItemInfoManager();