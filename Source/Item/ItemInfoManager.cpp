#include "ItemInfoManager.h"
#include "File.h"
#include "../Utils/StringUtils.h"
#include "../IO/Log.h"

ItemInfoManager::ItemInfoManager()
{
}

ItemInfoManager::~ItemInfoManager()
{
    for(auto& pItem : m_items) {
        SAFE_DELETE(pItem);
    }

    m_items.clear();
}

bool ItemInfoManager::Load(const string& filePath)
{
    File file;
    if(!file.Open(filePath)) {
        return false;
    }

    uint32 fileSize = file.GetSize();
    string fileData(fileSize, '\0');

    if(file.Read(fileData.data(), fileSize) != fileSize) {
        return false;
    }

    auto lines = Split(fileData, '\n');

    ItemInfo* pLastItem = nullptr;

    for(auto& line : lines) {
        if(line.empty() || line[0] == '#') {
            continue;
        }

        auto args = Split(line, '|');

        if(args[0] == "make_null") {
            int32 nullItemCount = ToInt(args[2]) - ToInt(args[1]);

            for(int32 i = 0; i < nullItemCount; i += 2) {
                ItemInfo* pItem = new ItemInfo();
                pItem->id = ToInt(args[1]) + i;
                pItem->name = "null_item" + std::stoi(args[1]) + i;
                pItem->textureFile = "tiles_page1.rttex";

                m_items.push_back(pItem);
                CreateDefaultSeedForItem(pItem);
            }
            continue;
        }

        if(args[0] == "add_item") {
            ItemInfo* pItem = new ItemInfo();

            pItem->id = (uint32)ToInt(args[1]);
            pItem->name = args[2];
            pItem->type = StrToItemType(args[3]);
            pItem->material = StrToItemMaterial(args[4]);
            pItem->textureFile = args[5];

            auto coords = Split(args[5], ',');
            pItem->textureX = (uint8)ToInt(coords[0]);
            pItem->textureY = (uint8)ToInt(coords[1]);

            pItem->visualEffect = StrToItemVisualEffect(args[7]);
            pItem->storage = StrToStorageType(args[8]);
            pItem->collisionType = StrToCollisionType(args[9]);
            pItem->hp = (uint8)ToInt(args[10]);
            pItem->restoreTime = ToInt(args[11]);

            m_items.push_back(pItem);
            pLastItem = pItem;
            CreateDefaultSeedForItem(pItem);
        }

        if(args[0] == "add_cloth") {
            ItemInfo* pItem = new ItemInfo();

            pItem->id = (uint32)ToInt(args[1]);
            pItem->name = args[2];
            pItem->material = StrToItemMaterial(args[3]);
            pItem->textureFile = args[4];

            auto coords = Split(args[5], ',');
            pItem->textureX = (uint8)ToInt(coords[0]);
            pItem->textureY = (uint8)ToInt(coords[1]);

            pItem->visualEffect = StrToItemVisualEffect(args[6]);
            pItem->storage = StrToStorageType(args[7]);
            pItem->bodyPart = StrToBodyPartType(args[8]);
        
            m_items.push_back(pItem);
            pLastItem = pItem;
            CreateDefaultSeedForItem(pItem);
        }

        if(args[0] == "set_seed") {
            if(!pLastItem) {
                continue;
            }

            ItemInfo* pSeed = m_items[pLastItem->id + 1];
            if(!pSeed) {
                continue;
            }

            pSeed->seed1 = (uint32)ToInt(args[1]);
            pSeed->seed2 = (uint32)ToInt(args[2]);

            //pSeed->seedBgColor;
            //pSeed->seedFgColor;
        }

        if(args[0] == "description") {
            if(!pLastItem) {
                continue;
            }

            pLastItem->description = args[1];
        }

        if(args[0] == "set_rarity") {
            if(!pLastItem) {
                continue;
            }

            ItemInfo* pSeed = m_items[pLastItem->id + 1];
            pLastItem->rarity = (uint32)ToInt(args[1]);
            pSeed->rarity = (uint32)ToInt(args[1]);
        }

        if(args[0] == "set_element") {
            if(!pLastItem) {
                continue;
            }

            pLastItem->element = StrToItemElement(args[1]);
        }

        if(args[0] == "set_flags") {
            if(!pLastItem) {
                continue;
            }

            auto flags = Split(args[1], ',');
            for(auto& flag : flags) {
                pLastItem->flags |= StrToItemFlag(flag);
            }
        }

        if(args[0] == "set_extra") {
            if(!pLastItem) {
                continue;
            }

            if(!args[1].empty()) {
                pLastItem->extraString = args[1];
            }

            if(!args[2].empty()) {
                pLastItem->animMS = (uint32)ToInt(args[2]);
            }
        }
    }

    m_itemCount = m_items.size();
    SetupItemExtras();
    return true;
}

/**
 * add ios & mac serialization
 */
bool ItemInfoManager::LoadFileHashes(const std::vector<string>& fileData)
{
    auto findHash = [&](const string& fileName) -> uint32
    {
        for(uint32 i = 0; i < fileData.size(); i += 2) {
            if(fileData[i] == fileName) {
                return ToUInt(fileData[i+1]);
            }
        }

        return 0;
    };

    for(auto& pItem : m_items) {
        uint32 textureFileHash = findHash("game/" + pItem->textureFile);

        if(textureFileHash == 0) {
            LOGGER_LOG_WARN("Unable to get FILE hash for %s %s, it wasnt set to 0 right?!", pItem->name.c_str(), pItem->textureFile.c_str());
        }
        else {
            pItem->textureHash = textureFileHash;
        }

        if(!pItem->extraString.empty()) {
            uint32 extraStringHash = findHash(pItem->extraString);

            if(extraStringHash == 0) {
                LOGGER_LOG_WARN("Unable to get EXTRA STRING hash for %s %s, it wasnt set to 0 right?!", pItem->name.c_str(), pItem->extraString.c_str());
            }
            else {
                pItem->extraString = extraStringHash;
            }
        }
    }
}

ItemInfo* ItemInfoManager::GetItemByID(uint32 itemID)
{
    if(itemID > m_itemCount) {
        return nullptr;
    }

    return m_items[itemID];
}

void ItemInfoManager::SetupItemExtras()
{
    for(uint32 i = 0; i < m_items.size(); i += 2) {
        ItemInfo* pItem = m_items[i];
        ItemInfo* pSeed = m_items[i + 1];

        pSeed->rarity = pItem->rarity;

        if(pItem->rarity != 0) {
            if(pItem->rarity == 999) {
                pSeed->growTime = 3600;
            }
            else {
                pSeed->growTime = pItem->rarity^3 + 30 * pItem->rarity;
            }
            continue;
        }

        if(pSeed->seed1 == 0 || pSeed->seed2 == 0) {
            pItem->rarity = 1;
            pSeed->rarity = 1;
            pSeed->growTime = 31;
            continue;
        }

        ItemInfo* pSeed1 = m_items[pSeed->seed1];
        ItemInfo* pSeed2 = m_items[pSeed->seed2];

        uint16 rarity = pSeed1->rarity + pSeed2->rarity;
        if(rarity > 999) {
            rarity = 999;
        }

        pItem->rarity = rarity;
        pSeed->rarity = rarity;

        if(rarity == 999) {
            pSeed->growTime = 3600;
            continue;
        }

        pSeed->growTime = rarity^3 + 30 * rarity;
    }
}

void ItemInfoManager::CreateDefaultSeedForItem(ItemInfo* pItem)
{
    if(!pItem) {
        return;
    }

    ItemInfo* pSeed = new ItemInfo();
    if(pItem->HasFlag(ITEM_FLAG_RANDGROW)) { pSeed->flags |= ITEM_FLAG_RANDGROW; }
    if(pItem->HasFlag(ITEM_FLAG_MOD)) { pSeed->flags |= ITEM_FLAG_MOD; }
    if(pItem->HasFlag(ITEM_FLAG_SEEDLESS)) { pSeed->flags |= ITEM_FLAG_SEEDLESS; }
    if(pItem->HasFlag(ITEM_FLAG_BETA)) { pSeed->flags |= ITEM_FLAG_BETA; }
    if(pItem->HasFlag(ITEM_FLAG_HOLIDAY)) { pSeed->flags |= ITEM_FLAG_HOLIDAY; }

    pSeed->seedBg = (uint8)((pItem->id / 2) % 16);
    pSeed->seedFg = (uint8)(pItem->id % 16);
    pSeed->treeBg = (uint8)((pItem->id / 2) % 8);
    pSeed->treeFg = (uint8)(pItem->id % 8);

    pSeed->id = pItem->id + 1;

    switch(pItem->id) {
        case 610: pSeed->name = "Magic Egg"; break;
        case 2034: pSeed->name = "Starseed"; break;
        case 2036: pSeed->name = "Galactic Starseed"; break;
        case 4454: pSeed->name = "Mutated Seed"; break;
        default: pSeed->name = pItem->name + " Seed";
    }

    pSeed->restoreTime = 2;
    pSeed->hp = 120;
    pSeed->type = ITEM_TYPE_SEED;
    pSeed->collisionType = COLLISION_NONE;
    pSeed->textureFile = pItem->textureFile;

    pSeed->description = "``Plant this to grow a `3" + pItem->name + " ``Tree";
}

ItemInfoManager* GetItemInfoManager() { return ItemInfoManager::GetInstance(); }
