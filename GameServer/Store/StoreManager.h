#pragma once

#include "Precompiled.h"
#include "../Player/GamePlayer.h"
#include "Utils/DialogBuilder.h"

enum eStoreTab
{
    STORE_TAB_MAIN_MENU,
    STORE_TAB_LOCKS_MENU,
    STORE_TAB_ITEMPACK_MENU,
    STORE_TAB_BIGITEMS_MENU,
    STORE_TAB_WEATHER_MENU,
    STORE_TAB_TOKEN_MENU,

    STORE_TAB_COUNT,
    STORE_TAB_NONE
};

struct StoreRule
{
    uint32 startTime = 0;
    uint32 endTime = 0;

    uint32 randomCount = 0;
    bool disabled = false;
};

struct StoreEntry
{
    string code;
    string name;
    string description;
    uint8 posX = 0;
    uint8 posY = 0;
    string banner;
    int32 cost = 0;

    bool isTab = false;
    eStoreTab parentTab = STORE_TAB_NONE;

    StoreRule rule;
    std::vector<uint32> items;
};

class StoreManager {
public:
    StoreManager();
    ~StoreManager();

public:
    static StoreManager* GetInstance()
    {
        static StoreManager instance;
        return &instance;
    }

public:
    bool Load(const string& filePath);

    eStoreTab GetTabTypeByCode(const string& entryCode);
    void NavigatePlayer(GamePlayer* pPlayer, eStoreTab storeTab);
    bool PurchaseItem(GamePlayer* pPlayer, const string& entryCode);
    string GivePurchasedItemsAndGetGivensAsStr(GamePlayer* pPlayer, const std::vector<uint32>& items);

    StoreEntry* GetStoreEntryByCodeFromTab(const string& entryCode, eStoreTab storeTab);
    StoreEntry* GetStoreEntryByCode(const string& entryCode);
    void BuildStore(GamePlayer* pPlayer, DialogBuilder& db, eStoreTab storeTab);

private:
    void BuildStoreEntry(GamePlayer* pPlayer, DialogBuilder& db, StoreEntry* pEntry);
    StoreEntry* GetEntryByCodeRaw(const string& entryCode);
    void HashEntries();

private:
    std::vector<StoreEntry> m_storeEntries[STORE_TAB_COUNT];
    std::unordered_map<uint32, StoreEntry*> m_storeSearch;
};

StoreManager* GetStoreManager();