#include "ItemInfoManager.h"
#include "../IO/File.h"
#include "../Utils/StringUtils.h"
#include "../IO/Log.h"
#include "../Proton/ProtonUtils.h"

ItemInfoManager::ItemInfoManager()
: m_version(ITEM_DATA_VERSION)
{
}

ItemInfoManager::~ItemInfoManager()
{
    Kill();
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
        file.Close();
        return false;
    }

    auto lines = Split(fileData, '\n');
    uint32 lastItemID = 0;

    for(auto& line : lines) {
        if(line.empty() || line[0] == '#') {
            continue;
        }

        auto args = Split(line.data(), line.size(), '|');

        if(args[0] == "make_null") {
            int32 nullItemCount = ToInt(args[2]) - ToInt(args[1]);

            for(int32 i = 0; i < nullItemCount; i += 2) {
                ItemInfo item;
                item.id = ToInt(args[1]) + i;
                item.name = "null_item" + ToString(item.id + 1);
                item.textureFile = "tiles_page1.rttex";

                m_items.push_back(std::move(item));
                CreateDefaultSeedForItem(&m_items.back());
            }
            continue;
        }

        if(args[0] == "add_item") {
            ItemInfo item;

            item.id = ToUInt(args[1]);
            item.name = args[2];
            item.type = StrToItemType(args[3]);
            item.material = StrToItemMaterial(args[4]);
            item.textureFile = args[5];

            item.textureX = (uint8)ToUInt(args[6]);
            item.textureY = (uint8)ToUInt(args[7]);

            item.visualEffect = StrToItemVisualEffect(args[8]);
            item.storage = StrToStorageType(args[9]);
            item.collisionType = StrToCollisionType(args[10]);
            item.hp = (uint8)ToUInt(args[11]) * 6;
            item.restoreTime = ToInt(args[12]);

            lastItemID = item.id;
            m_items.push_back(std::move(item));
            CreateDefaultSeedForItem(&m_items[lastItemID]);
        }

        if(args[0] == "add_cloth") {
            ItemInfo item;

            item.id = ToUInt(args[1]);
            item.name = args[2];
            item.material = StrToItemMaterial(args[3]);
            item.textureFile = args[4];

            item.textureX = (uint8)ToUInt(args[5]);
            item.textureY = (uint8)ToUInt(args[6]);

            item.visualEffect = StrToItemVisualEffect(args[7]);
            item.storage = StrToStorageType(args[8]);
            item.bodyPart = StrToBodyPartType(args[9]);

            item.type = ITEM_TYPE_CLOTHES;
        
            lastItemID = item.id;
            m_items.push_back(std::move(item));
            CreateDefaultSeedForItem(&m_items[lastItemID]);
        }

        if(args[0] == "set_seed") {
            if(m_items.size() < lastItemID + 1) {
                continue;
            }

            ItemInfo& seed = m_items[lastItemID + 1];

            seed.seed1 = (uint16)ToUInt(args[1]);
            seed.seed2 = (uint16)ToUInt(args[2]);

            seed.seedBgColor = ToColor(args[3], ',');
            seed.seedFgColor = ToColor(args[4], ',');
        }

        if(args[0] == "description") {
            if(m_items.size() < lastItemID) {
                continue;
            }

            m_items[lastItemID].description = args[1];
        }

        if(args[0] == "set_element") {
            if(m_items.size() < lastItemID) {
                continue;
            }

            m_items[lastItemID].element = StrToItemElement(args[1]);
        }

        if(args[0] == "set_flags") {
            if(m_items.size() < lastItemID) {
                continue;
            }

            for(uint16 i = 1; i < args.size(); ++i) {
                m_items[lastItemID].flags |= StrToItemFlag(args[i]);
            }
        }

        if(args[0] == "set_flags2") {
            if(m_items.size() < lastItemID) {
                continue;
            }

            for(uint16 i = 1; i < args.size(); ++i) {
                m_items[lastItemID].flags2 |= StrToFlags2(args[i]);
            }
        }

        if(args[0] == "set_fx_flags") {
            if(m_items.size() < lastItemID) {
                continue;
            }

            uint32 i = 1;
            while (i < args.size())
            {
                uint32 convFlag = StrToFxFlag(args[i++]);
                m_items[lastItemID].fxFlags |= convFlag;
            
                if (convFlag == ITEM_FX_FLAG_MULTI_ANIM)
                {
                    while (i < args.size() && args[i] != "MULTI_ANIM_END")
                    {
                        m_items[lastItemID].multiAnim1 += args[i] + "|";
                        ++i;
                    }
                    ++i;
                    continue;
                }
            
                if (convFlag == ITEM_FX_FLAG_MULTI_ANIM2)
                {
                    while (i < args.size() && args[i] != "MULTI_ANIM2_END")
                    {
                        m_items[lastItemID].multiAnim2 += args[i] + "|";
                        ++i;
                    }
                    ++i;
                    continue;
                }
            
                if (convFlag == ITEM_FX_FLAG_DUAL_LAYER)
                {
                    if (i >= args.size()) {
                        break;
                    }
            
                    auto dualLayer = Split(args[i++], ',');
                    m_items[lastItemID].dualAnimLayer.x = ToInt(dualLayer[0]);
                    m_items[lastItemID].dualAnimLayer.y = ToInt(dualLayer[1]);
                    continue;
                }
            
                if (convFlag == ITEM_FX_FLAG_OVERLAY_OBJECT)
                {
                    if (i >= args.size()) {
                        break;
                    }

                    m_items[lastItemID].overlayTextureFile = args[i++];
                    continue;
                }
            
                if (convFlag == ITEM_FX_FLAG_RENDER_FX_VARIANT_VERSION)
                {
                    if (i >= args.size()) {
                        break;
                    }

                    m_items[lastItemID].variantVersionItem = ToInt(args[i++]);
                    continue;
                }
            }
        }

        if(args[0] == "set_extra") {
            if(m_items.size() < lastItemID) {
                continue;
            }

            if(!args[1].empty()) {
                m_items[lastItemID].extraString = args[1];
            }

            if(!args[2].empty()) {
                m_items[lastItemID].animMS = ToInt(args[2]);
            }
        }

        if(args[0] == "set_max_hold") {
            if(m_items.size() < lastItemID) {
                continue;
            }

            m_items[lastItemID].maxCanHold = ToUInt(args[1]);
        }

        if(args[0] == "set_custom_punch") {
            if(m_items.size() < lastItemID) {
                continue;
            }

            m_items[lastItemID].customizedPunchParameters = args[1];
        }

        if(args[0] == "set_config_name") {
            if(m_items.size() < lastItemID) {
                continue;
            }

            m_items[lastItemID].configName = args[1];
        }

        if(args[0] == "set_rarity") {
            if(m_items.size() < lastItemID) {
                continue;
            }

            m_items[lastItemID].rarity = (int16)ToInt(args[1]);
        }
    }

    file.Close();

    m_itemCount = m_items.size();
    return true;
}

bool ItemInfoManager::LoadByItemsDat(const string& filePath)
{
    File file;
    if(!file.Open(filePath)) {
        return false;
    }

    uint32 fileSize = file.GetSize();
    uint8* pData = new uint8[fileSize];

    if(file.Read(pData, fileSize) != fileSize) {
        file.Close();
        SAFE_DELETE_ARRAY(pData);
        return false;
    }

    MemoryBuffer memBuffer(pData, fileSize);
    memBuffer.Read(m_version);
    memBuffer.Read(m_itemCount);

    m_items.resize(m_itemCount);

    for(uint32 i = 0; i < m_itemCount; ++i) {
        m_items[i].Serialize(memBuffer, false, m_version);
    }

    file.Close();
    SAFE_DELETE_ARRAY(pData);
    return true;
}

bool ItemInfoManager::LoadWikiData(const string& filePath)
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
    for(auto& line : lines) {
        if(line.empty() || line[0] == '#') {
            continue;
        }

        auto args = Split(line, '|');
        if(args[0] == "set_wiki") {
            uint32 itemID = ToUInt(args[1]);

            ItemInfo* pItem = GetItemByID(itemID);
            if(!pItem) {
                LOGGER_LOG_ERROR("Tried add wiki data on %d but non exists?", itemID);
                continue;
            }

            ItemInfo* pSeed = GetItemByID(itemID + 1);
            if(pSeed) {
                if(pSeed->seed1 == 0 && pSeed->seed2 == 0) {
                    pSeed->seed1 = ToUInt(args[2]);
                    pSeed->seed2 = ToUInt(args[3]);
                }
            }
            else {
                LOGGER_LOG_ERROR("Tried to set wiki seed data on %d but seed not exists!", itemID);
            }

            pItem->element = StrToItemElement(args[4]);

            if(pItem->description.empty()) {
                pItem->description = args[5];
            }
        }

        /*if(args[0] == "set_combine") {

        }*/
    }

    SetupItemExtras();
    return true;
}

void ItemInfoManager::Kill()
{
    SAFE_DELETE_ARRAY(m_itemDataMp3.pItemData);
    SAFE_DELETE_ARRAY(m_itemDataOgg.pItemData);

    m_items.clear();
}

void ItemInfoManager::LoadFileHashes(const std::unordered_map<string, uint32>& hashData, bool forOgg)
{
    auto FindHash = [&](const string& fileName) -> uint32
    {
        auto it = hashData.find(fileName);
        if(it != hashData.end()) {
            return it->second;
        }

        return 0;
    };

    for(auto& item : m_items) {
        uint32 textureFileHash = FindHash("game/" + item.textureFile);

        if(item.textureHash == 0) {
            item.textureHash = textureFileHash;
        }

        if(!item.extraString.empty()) {
            string filePath = item.extraString;
            if(forOgg) {
                ReplaceString(filePath, "mp3", "ogg");
            }

            uint32 extraStringHash = FindHash(filePath);

            if(item.extraStringHash == 0) {
                item.extraStringHash = extraStringHash;
            }
        }
    }
}

void ItemInfoManager::SaveToClientData(bool forOgg)
{
    MemoryBuffer memSizeBuffer;
    memSizeBuffer.Seek(sizeof(m_version) + sizeof(m_itemCount));

    for(auto& item : m_items) {
        item.Serialize(memSizeBuffer, true, m_version);
    }

    uint32 memSize = memSizeBuffer.GetOffset();
    uint8* pData = new uint8[memSize];

    MemoryBuffer memBuffer(pData, memSize);
    memBuffer.Write(m_version);
    memBuffer.Write(m_itemCount);

    for(auto& item : m_items) {
        item.Serialize(memBuffer, true, m_version);
    }

    if(forOgg) {
        m_itemDataOgg.pItemData = pData;
        m_itemDataOgg.size = memSize;
        m_itemDataOgg.hash = Proton::HashString((const char*)pData, memSize);
    }
    else {
        m_itemDataMp3.pItemData = pData;
        m_itemDataMp3.size = memSize;
        m_itemDataMp3.hash = Proton::HashString((const char*)pData, memSize);
    }
}

ItemInfo* ItemInfoManager::GetItemByID(uint32 itemID)
{
    if(itemID > m_itemCount) {
        return nullptr;
    }

    return &m_items[itemID];
}

ItemInfo* ItemInfoManager::GetItemByName(const string& name)
{
    if(name.empty()) {
        return nullptr;
    }

    string searchName = ToLower(name);

    for(auto& item : m_items) {
        if(ToLower(item.name) == searchName) {
            return &item;
        }
    }

    return nullptr;
}

ItemsClientData& ItemInfoManager::GetClientData(uint8 platformType)
{
    if(platformType == Proton::PLATFORM_ID_ANDROID) {
        return m_itemDataMp3;
    }
    return m_itemDataOgg;
}

void ItemInfoManager::SetupItemExtras()
{
    for(uint32 i = 0; i < m_items.size(); i += 2) {
        ItemInfo* pItem = GetItemByID(i);
        ItemInfo* pSeed = GetItemByID(i + 1);

        if(!pItem || !pSeed) {
            continue;
        }

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

        ItemInfo* pSeed1 = GetItemByID(pSeed->seed1);
        ItemInfo* pSeed2 = GetItemByID(pSeed->seed2);

        if(!pSeed1 || !pSeed2) {
            continue;
        }

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

        pSeed->growTime = rarity * rarity * rarity + 30 * rarity;
    }
}

void ItemInfoManager::CreateDefaultSeedForItem(ItemInfo* pItem)
{
    if(!pItem) {
        return;
    }

    ItemInfo seed;
    if(pItem->HasFlag(ITEM_FLAG_RANDGROW)) { seed.flags |= ITEM_FLAG_RANDGROW; }
    if(pItem->HasFlag(ITEM_FLAG_MOD)) { seed.flags |= ITEM_FLAG_MOD; }
    if(pItem->HasFlag(ITEM_FLAG_SEEDLESS)) { seed.flags |= ITEM_FLAG_SEEDLESS; }
    if(pItem->HasFlag(ITEM_FLAG_BETA)) { seed.flags |= ITEM_FLAG_BETA; }
    if(pItem->HasFlag(ITEM_FLAG_HOLIDAY)) { seed.flags |= ITEM_FLAG_HOLIDAY; }

    seed.seedBg = (uint8)((pItem->id / 2) % 16);
    seed.seedFg = (uint8)(pItem->id % 16);
    seed.treeBg = (uint8)((pItem->id / 2) % 8);
    seed.treeFg = (uint8)(pItem->id % 8);

    seed.id = pItem->id + 1;

    switch(pItem->id) {
        case ITEM_ID_MAGIC_EGG: seed.name = "Magic Egg"; break;
        case ITEM_ID_COMET_DUST: seed.name = "Starseed"; break;
        case ITEM_ID_ANTIMATTER_DUST: seed.name = "Galactic Starseed"; break;
        case ITEM_ID_MUTATED_SEED_CORE: seed.name = "Mutated Seed"; break;
        default: seed.name = pItem->name + " Seed";
    }

    seed.restoreTime = 2;
    seed.hp = 120;
    seed.type = ITEM_TYPE_SEED;
    seed.collisionType = COLLISION_NONE;
    seed.textureFile = pItem->textureFile;

    seed.description = "``Plant this to grow a `3" + pItem->name + " ``Tree";
    m_items.push_back(seed);
}

ItemInfoManager* GetItemInfoManager() { return ItemInfoManager::GetInstance(); }
