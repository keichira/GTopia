#include "ItemInfoManager.h"
#include "../IO/File.h"
#include "../Utils/StringUtils.h"
#include "../IO/Log.h"
#include "../Proton/ProtonUtils.h"
#include "../Math/Math.h"
#include "../Utils/ZLibUtils.h"
#include "../Utils/ConfigDB.h"

uint16 GetSupportedItemDataVersion(float gameVersion)
{
    for(auto& version : sItemDataVersionMap)
    {
        if(gameVersion >= version.first)
            return version.second;
    }

    return 11;
}

uint16 GetMinRequiredItemDataVersion(float a, float b, float c, float d)
{
    float minV = Min(Min(a, b), Min(c, d));
    return GetSupportedItemDataVersion(minV);
}

uint16 GetMaxRequiredItemDataVersion(float a, float b, float c, float d)
{
    float maxV = Max(Max(a, b), Max(c, d));
    return GetSupportedItemDataVersion(maxV);
}

uint32 StrToConsumableFlag(const string &str)
{
    static const std::unordered_map<string, uint32> itemConsumableFlag
    {
        { "NEED_TARGET", CONSUMABLE_FLAG_NEED_TARGET },
        { "SELF_ONLY", CONSUMABLE_FLAG_SELF_ONLY },
        { "NEED_TILE", CONSUMABLE_FLAG_NEED_TILE },
        { "EQUIP", CONSUMABLE_FLAG_EQUIP }
    };

    auto it = itemConsumableFlag.find(str);
    if(it != itemConsumableFlag.end()) {
        return it->second;
    }

    return 0;
}

ItemInfoManager::ItemInfoManager()
: m_version(0)
{
}

ItemInfoManager::~ItemInfoManager()
{
    Kill();
}

bool ItemInfoManager::Load(const string& filePath)
{
    ConfigDB cfg;
    if(!cfg.Load(filePath))
        return false;

    uint32 lastItemID = 0;

    for(auto& line : cfg.Lines())
    {
        const string& key = line.GetString(0);

        if(key == "make_null")
        {
            if(!line.Require(2))
                return false;

            int32 min = line.GetInt(1);
            int32 max = line.GetInt(2);

            if(min < 0 || max < 0 || max - min < 0)
            {
                LOGGER_LOG_ERROR("Error happened on line %d", line.lineNumber);
                return false;
            }

            for(uint32 i = 0; i < (max - min); i += 2)
            {
                ItemInfo item;
                item.id = min + i;
                item.name = "null_item" + ToString(item.id + 1);
                item.textureFile = "tiles_page1.rttex";

                m_items.push_back(std::move(item));
                CreateDefaultSeedForItem(&m_items.back());
            }

            continue;
        }

        if(key == "add_item")
        {
            if(!line.Require(13))
                return false;

            ItemInfo item;
            item.id = line.GetUInt(1);
            item.name = line.GetString(2);
            item.type = StrToItemType(line.GetString(3));
            item.material = StrToItemMaterial(line.GetString(4));
            item.textureFile = line.GetString(5);

            item.textureX = (uint8)line.GetUInt(6);
            item.textureY = (uint8)line.GetUInt(7);

            item.visualEffect = StrToItemVisualEffect(line.GetString(8));
            item.storage = StrToStorageType(line.GetString(9));
            item.collisionType = StrToCollisionType(line.GetString(10));
            item.hp = line.GetUInt(11) * 6;
            item.restoreTime = line.GetInt(12);
            item.farmablity = line.GetInt(13);
            item.textureHash = line.GetUInt(14);

            lastItemID = item.id;
            m_items.push_back(std::move(item));
            CreateDefaultSeedForItem(&m_items[lastItemID]);
            continue;
        }

        if(key == "add_cloth")
        {
            if(!line.Require(9))
                return false;

            ItemInfo item;

            item.id = line.GetUInt(1);
            item.name = line.GetString(2);
            item.material = StrToItemMaterial(line.GetString(3));
            item.textureFile = line.GetString(4);

            item.textureX = (uint8)line.GetUInt(5);
            item.textureY = (uint8)line.GetUInt(6);

            item.visualEffect = StrToItemVisualEffect(line.GetString(7));
            item.storage = StrToStorageType(line.GetString(8));
            item.bodyPart = StrToBodyPartType(line.GetString(9));

            item.type = ITEM_TYPE_CLOTHES;
        
            lastItemID = item.id;
            m_items.push_back(std::move(item));
            CreateDefaultSeedForItem(&m_items[lastItemID]);
            continue;
        }

        if(key == "set_seed")
        {
            if(lastItemID + 1 > m_items.size())
                continue;

            if(!line.Require(4))
                return false;

            ItemInfo& seed = m_items[lastItemID + 1];

            seed.seed1 = line.GetUInt(1);
            seed.seed2 = line.GetUInt(2);

            seed.seedBgColor = ToColor(line.GetString(3), ',');
            seed.seedFgColor = ToColor(line.GetString(4), ',');
            continue;
        }

        if(key == "description")
        {
            if(m_items.size() < lastItemID)
                continue;

            m_items[lastItemID].description = line.GetString(1);
        }

        if(key == "set_element")
        {
            if(m_items.size() < lastItemID)
                continue;

            m_items[lastItemID].element = StrToItemElement(line.GetString(1));
            continue;
        }

        if(key == "set_flags")
        {
            if(m_items.size() < lastItemID)
                continue;
                
            for(uint16 i = 1; i < line.GetArgSize(); ++i) 
            {
                m_items[lastItemID].flags |= StrToItemFlag(line.GetString(i));
            }

            continue;
        }

        if(key == "set_flags2")
        {
            if(m_items.size() < lastItemID)
                continue;
                
            for(uint16 i = 1; i < line.GetArgSize(); ++i) 
            {
                m_items[lastItemID].flags2 |= StrToFlags2(line.GetString(i));
            }

            continue;
        }

        if(key == "set_fx_flags")
        {
            if(m_items.size() < lastItemID)
                continue;

            uint32 i = 1;
            while (i < line.GetArgSize())
            {
                uint32 convFlag = StrToFxFlag(line.GetString(i++));
                m_items[lastItemID].fxFlags |= convFlag;
            
                if(convFlag == ITEM_FX_FLAG_MULTI_ANIM)
                {
                    while (i < line.GetArgSize() && line.GetString(i) != "MULTI_ANIM_END")
                    {
                        m_items[lastItemID].multiAnim1 += line.GetString(i) + "|";
                        ++i;
                    }
                    ++i;
                    continue;
                }
            
                if(convFlag == ITEM_FX_FLAG_MULTI_ANIM2)
                {
                    while (i < line.GetArgSize() && line.GetString(i) != "MULTI_ANIM2_END")
                    {
                        m_items[lastItemID].multiAnim2 += line.GetString(i) + "|";
                        ++i;
                    }
                    ++i;
                    continue;
                }
            
                if(convFlag == ITEM_FX_FLAG_DUAL_LAYER)
                {
                    if(i >= line.GetArgSize())
                        break;
            
                    auto dualLayer = Split(line.GetString(i++), ',');
                    m_items[lastItemID].dualAnimLayer.x = ToInt(dualLayer[0]);
                    m_items[lastItemID].dualAnimLayer.y = ToInt(dualLayer[1]);
                    continue;
                }
            
                if (convFlag == ITEM_FX_FLAG_OVERLAY_OBJECT)
                {
                    if (i >= line.GetArgSize()) {
                        break;
                    }

                    m_items[lastItemID].overlayTextureFile = line.GetString(i++);
                    continue;
                }
            
                if (convFlag == ITEM_FX_FLAG_RENDER_FX_VARIANT_VERSION)
                {
                    if (i >= line.GetArgSize()) {
                        break;
                    }

                    m_items[lastItemID].variantVersionItem = line.GetInt(i++);
                    continue;
                }
            }

            continue;
        }

        if(key == "set_extra")
        {
            if(m_items.size() < lastItemID)
                continue;

            m_items[lastItemID].extraString = line.GetString(1);
            m_items[lastItemID].animMS = line.GetInt(2, 200);
            m_items[lastItemID].extraStringHash = line.GetUInt(3);
            continue;
        }

        if(key == "set_max_hold")
        {
            if(m_items.size() < lastItemID)
                continue;

            m_items[lastItemID].maxCanHold = line.GetUInt(1, 1);
            continue;
        }

        if(key == "set_custom_punch")
        {
            if(m_items.size() < lastItemID)
                continue;

            m_items[lastItemID].customizedPunchParameters = line.GetString(1);
            continue;
        }

        if(key == "set_config_name")
        {
            if(m_items.size() < lastItemID)
                continue;

            m_items[lastItemID].configName = line.GetString(1);
            continue;
        }

        if(key == "set_rarity")
        {
            if(m_items.size() < lastItemID)
                continue;

            m_items[lastItemID].rarity = line.GetInt(1, 1);

            if(m_items.size() >= lastItemID + 1) 
            {
                m_items[lastItemID + 1].rarity = m_items[lastItemID].rarity;
            }
            continue;
        }
    }

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

    for(uint32 i = 0; i < m_itemCount; ++i) 
    {
        m_items[i].Serialize(memBuffer, false, m_version);
    }

    file.Close();
    SAFE_DELETE_ARRAY(pData);
    return true;
}

bool ItemInfoManager::LoadWikiData(const string& filePath)
{
    ConfigDB cfg;
    if(!cfg.Load(filePath))
        return false;

    for(auto& line : cfg.Lines())
    {
        const string& key = line.GetString(0);

        if(key == "set_wiki")
        {
            if(!line.Require(5))
                return false;

            uint32 itemID = line.GetUInt(1);
            if(itemID == ITEM_ID_BLANK)
                continue;

            ItemInfo* pSeed = GetItemByID(itemID + 1);

            if(pSeed && pSeed->seed1 != 0 && pSeed->seed2 != 0)
            {
                pSeed->seed1 = line.GetUInt(2);
                pSeed->seed2 = line.GetUInt(3);
            }

            ItemInfo* pItem = GetItemByID(itemID);
            if(!pItem)
                continue;

            if(pItem->element == ITEM_ELEMENT_NONE)
            {
                pItem->element = StrToItemElement(line.GetString(4));
            }

            if(pItem->description == "No info.")
            {
                pItem->description = line.GetString(5);
            }

            continue;
        }
    }

    return true;
}

bool ItemInfoManager::LoadConsumableData(const string& filePath)
{
    ConfigDB cfg;
    if(!cfg.Load(filePath))
        return false;

    uint32 lastItemID = 0;

    for(auto& line : cfg.Lines())
    {
        const string& key = line.GetString(0);

        if(key == "add_consume")
        {
            if(!line.Require(7))
                return false;

            if(line.GetUInt(1) == 0)
            {
                LOGGER_LOG_ERROR("Error on line %d something wrong with itemID", line.lineNumber);
                return false;
            }

            ConsumableInfo info;
            info.itemID = line.GetUInt(1);
            info.consumableType = line.GetUInt(2);
            info.requiredAmount = line.GetUInt(3);
            info.rewardItemID = line.GetUInt(4);
            info.rewardCount = line.GetUInt(5);
            info.successMessage = line.GetString(6);
            info.failMessage = line.GetString(7);

            lastItemID = info.itemID;
            m_consumeData.insert_or_assign(info.itemID, std::move(info));
            continue;
        }

        if(key == "set_flags")
        {
            if(!line.Require(1))
                return false;

            ConsumableInfo* pConfig = GetConsumableInfo(lastItemID);
            if(!pConfig)
            {
                LOGGER_LOG_ERROR("Error on line %d, failed to find lastitem %d", line.lineNumber, lastItemID);
                return false;
            }

            for(uint8 i = 1; i < line.GetArgSize(); ++i)
            {
                pConfig->flags |= StrToConsumableFlag(line.GetString(i));
            }

            continue;
        }
    }

    return true;
}

bool ItemInfoManager::LoadBattlePetData(const string& filePath)
{
    ConfigDB cfg;
    if(!cfg.Load(filePath))
        return false;

    uint32 lastItemID = 0;

    for(auto& line : cfg.Lines())
    {
        const string& key = line.GetString(0);

        if(key == "add_pet")
        {
            if(!line.Require(12))
                return false;

            BattlePetInfo info;
            info.itemID = line.GetUInt(1);
            info.name = line.GetString(2);
            info.subName = line.GetString(3);
            info.endName = line.GetString(4);
            info.powerName = line.GetString(5);
            info.element = StrToItemElement(line.GetString(6));
            info.ability = line.GetUInt(7);
            info.abilityVal = line.GetUInt(8);
            info.cooldownSec = line.GetUInt(9);
            info.superAbility = line.GetUInt(10);
            info.superAbilityVal = line.GetUInt(11);
            info.superAbilityDurationSec = line.GetUInt(12);
            info.powerParticle = line.GetUInt(13);
            info.hitParticle = line.GetUInt(14);
            info.powerSound = line.GetString(15);
            info.hitSound = line.GetString(16);

            lastItemID = info.itemID;
            m_battlePetData.insert_or_assign(info.itemID, std::move(info));
            continue;
        }

        if(key == "set_description")
        {
            if(BattlePetInfo* pPetInfo = GetBattlePetInfo(lastItemID))
            {
                pPetInfo->description = line.GetString(1);
            }

            continue;
        }
    }

    return true;
}

void ItemInfoManager::Kill()
{
    for(uint32 i = 0; i < MAX_SUPPORTED_ITEM_DATA_VERSION; ++i)
    {
        SAFE_DELETE_ARRAY(m_itemDataMp3[i].pItemData);
        SAFE_DELETE_ARRAY(m_itemDataOgg[i].pItemData);
    }

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

void ItemInfoManager::SaveToClientData(bool forOgg, uint16 minVersion, uint16 maxVersion)
{
    if(m_version != 0)
    {
        minVersion = m_version;
        maxVersion = m_version + 1;
    }

    for(uint16 i = minVersion; i < maxVersion; ++i)
    {
        MemoryBuffer memSizeBuffer;
        memSizeBuffer.Seek(sizeof(i) + sizeof(m_itemCount));
    
        for(auto& item : m_items) 
        {
            item.Serialize(memSizeBuffer, true, i);
        }

        uint32 memSize = memSizeBuffer.GetOffset();
        uint8* pData = new uint8[memSize];
    
        MemoryBuffer memBuffer(pData, memSize);
        memBuffer.Write(i);
        memBuffer.Write(m_itemCount);
    
        for(auto& item : m_items) 
        {
            item.Serialize(memBuffer, true, i);
        }

        uint32 compressSize = 0;
        uint8* pCompress = zLibDefalteToMemory(pData, memSize, compressSize);

        if(forOgg)
        {
            m_itemDataOgg[i].pItemData = pCompress;
            m_itemDataOgg[i].size = memSize;
            m_itemDataOgg[i].compressSize = compressSize;
            m_itemDataOgg[i].hash = Proton::HashString((const char*)pData, memSize);
        }
        else 
        {
            m_itemDataMp3[i].pItemData = pCompress;
            m_itemDataMp3[i].size = memSize;
            m_itemDataMp3[i].compressSize = compressSize;
            m_itemDataMp3[i].hash = Proton::HashString((const char*)pData, memSize);
        }

        SAFE_DELETE_ARRAY(pData);
    }
}

uint32 ItemInfoManager::GetBaseItemID(uint32 itemID)
{
    ItemInfo* pItem = GetItemByID(itemID);
    if(!pItem) {
        return itemID;
    }

    if(!pItem->HasFlag(ITEM_FLAG_RANDGROW)) {
        return itemID;
    }

    uint8 isSeed = itemID % 2;

    if(itemID >= ITEM_ID_CUDDLY_BUNNY && itemID <= ITEM_ID_PSYCHOTIC_BUNNY_SEED) return ITEM_ID_CUDDLY_BUNNY + isSeed;
    if(itemID >= ITEM_ID_RED_GROWSABER && itemID <= ITEM_ID_DOUBLE_GROWSABER_SEED) return ITEM_ID_RED_GROWSABER + isSeed;
    if(itemID >= ITEM_ID_FASHIONABLE_DRESS_BLUE && itemID <= ITEM_ID_FASHIONABLE_DRESS_YELLOW_SEED) return ITEM_ID_FASHIONABLE_DRESS_BLUE + isSeed;
    if(itemID >= ITEM_ID_STYLIN_DRESS_BLUE && itemID <= ITEM_ID_STYLIN_DRESS_YELLOW_SEED) return ITEM_ID_STYLIN_DRESS_BLUE + isSeed;
    if(itemID >= ITEM_ID_NUMBER_0 && itemID <= ITEM_ID_NUMBER_9_SEED) return ITEM_ID_NUMBER_0 + isSeed;
    if(itemID >= ITEM_ID_PAINT_BUCKET_RED && itemID <= ITEM_ID_PAINTBRUSH_SEED) return ITEM_ID_PAINT_BUCKET_RED + isSeed;
    if(itemID >= ITEM_ID_RED_CRAYON && itemID <= ITEM_ID_PURPLE_CRAYON_SEED) return ITEM_ID_RED_CRAYON + isSeed;
    if(itemID >= ITEM_ID_SUPER_SHIRT_RED && itemID <= ITEM_ID_SUPER_EYE_MASK_BLACK_SEED) return ITEM_ID_SUPER_SHIRT_RED + isSeed;
    if(itemID >= ITEM_ID_SUPER_LOGO_SKULL && itemID <= ITEM_ID_SUPER_LOGO_ECHO_SEED) return ITEM_ID_SUPER_LOGO_SKULL + isSeed;
    if(itemID >= ITEM_ID_SUPERPOWER_HEAT_VISION && itemID <= ITEM_ID_SUPERPOWER_OVERHEAT_SEED) return ITEM_ID_SUPERPOWER_HEAT_VISION + isSeed;
    if(itemID >= ITEM_ID_SUPERPOWER_ICE_SHARDS && itemID <= ITEM_ID_SUPERPOWER_FROZEN_MIRROR_SEED) return ITEM_ID_SUPERPOWER_ICE_SHARDS + isSeed;
    if(itemID >= ITEM_ID_TANGRAM_BLOCK_A && itemID <= ITEM_ID_TANGRAM_BLOCK_L_SEED) return ITEM_ID_TANGRAM_BLOCK_A + isSeed;
    if(itemID >= ITEM_ID_CARD_BLOCK_SPADE && itemID <= ITEM_ID_CARD_BLOCK_DIAMOND_SEED) return ITEM_ID_CARD_BLOCK_SPADE + isSeed;
    if(itemID >= ITEM_ID_SUPERPOWER_SUPER_STRENGTH && itemID <= ITEM_ID_SUPERPOWER_REGENERATION_SEED) return ITEM_ID_SUPERPOWER_SUPER_STRENGTH + isSeed;
    if(itemID >= ITEM_ID_SUPERPOWER_SHOCKING_FIST && itemID <= ITEM_ID_SUPERPOWER_RESUSCITATE_SEED) return ITEM_ID_SUPERPOWER_SHOCKING_FIST + isSeed;
    if(itemID >= ITEM_ID_STATUE_BLOCK && itemID <= ITEM_ID_STATUE_NOSE_SEED) return ITEM_ID_STATUE_BLOCK + isSeed;
    if(itemID >= ITEM_ID_WIZARD_HAT && itemID <= ITEM_ID_CURSED_WIZARD_HAT_SEED) return ITEM_ID_WIZARD_HAT + isSeed;
    if(itemID >= ITEM_ID_HIGH_HEELS_BLUE && itemID <= ITEM_ID_HIGH_HEELS_YELLOW_SEED) return ITEM_ID_HIGH_HEELS_BLUE + isSeed;
    if(itemID >= ITEM_ID_FASHION_PURSE_BLUE && itemID <= ITEM_ID_FASHION_PURSE_YELLOW_SEED) return ITEM_ID_FASHION_PURSE_BLUE + isSeed;
    if(itemID >= ITEM_ID_NUMBER_BLOCK_0 && itemID <= ITEM_ID_NUMBER_BLOCK_9_SEED) return ITEM_ID_NUMBER_BLOCK_0 + isSeed;
    if(itemID >= ITEM_ID_SURGICAL_SPONGE && ITEM_ID_SURGICAL_STITCHES_SEED) return ITEM_ID_SURGICAL_SPONGE + isSeed;
    if(itemID >= ITEM_ID_SURGICAL_PINS && ITEM_ID_SURGICAL_LAB_KIT_SEED) return ITEM_ID_SURGICAL_SPONGE + isSeed;

    return itemID;
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

ItemInfo* ItemInfoManager::GetSpliceInfo(uint16 seed1, uint16 seed2)
{
    uint16 a = Min(seed1, seed2);
    uint16 b = Max(seed1, seed2);

    uint32 key = (uint32(a) << 16) | uint32(b);

    auto it = m_spliceData.find(key);
    if (it == m_spliceData.end())
        return nullptr;

    return GetItemByID(it->second);
}

ConsumableInfo* ItemInfoManager::GetConsumableInfo(uint32 itemID)
{
    auto it = m_consumeData.find(itemID);
    if(it != m_consumeData.end())
        return &it->second;

    return nullptr;
}

BattlePetInfo* ItemInfoManager::GetBattlePetInfo(int32 itemID)
{
    auto it = m_battlePetData.find(itemID);
    if(it != m_battlePetData.end())
        return &it->second;

    return nullptr;
}

ItemsClientData* ItemInfoManager::GetClientData(uint8 platformType, float gameVersion)
{
    uint16 supportedVersion = m_version != 0 ? m_version : GetSupportedItemDataVersion(gameVersion);

    if(platformType == Proton::PLATFORM_ID_ANDROID) 
    {
        return &m_itemDataMp3[supportedVersion];
    }

    return &m_itemDataOgg[supportedVersion];
}

uint32 ItemInfoManager::GetItemRarity(uint32 itemID)
{
    ItemInfo* pItem = GetItemByID(itemID);
    if(!pItem) {
        return 0;
    }

    if(pItem->rarity != 0) {
        return pItem->rarity;
    }

    ItemInfo* pBaseItem = GetItemByID(GetBaseItemID(pItem->id));
    if(!pBaseItem) {
        return 1;
    }

    if(pBaseItem->seed1 == 0 && pBaseItem->seed2 == 0) {
        return 1;
    }

    uint16 rarity1 = pBaseItem->seed1 != 0 ? GetItemRarity(pBaseItem->seed1) : 0;
    uint16 rarity2 = pBaseItem->seed2 != 0 ? GetItemRarity(pBaseItem->seed2) : 0;

    uint32 rarity = rarity1 + rarity2;

    if(rarity > 999) {
        rarity = 999;
    }

    return rarity;
}

void ItemInfoManager::SetupItemExtras()
{
    for(uint32 i = 1; i < m_items.size(); i += 2)
    {
        ItemInfo* pSeed = GetItemByID(i);
        if(!pSeed)
            continue;
        
        uint16 minV = Min(pSeed->seed1, pSeed->seed2);
        uint16 maxV = Max(pSeed->seed1, pSeed->seed2);
        
        uint32 key = (uint32(minV) << 16) | uint32(maxV);
        m_spliceData.insert_or_assign(key, pSeed->id);

        ItemInfo* pItem = GetItemByID(i - 1);
        if(pItem->rarity == 0)
        {
            pSeed->rarity = GetItemRarity(pSeed->id);
            pItem->rarity = pSeed->rarity;
        }
        else
        {
            pSeed->rarity = pItem->rarity;
        }

        if(pSeed->rarity == 999) 
        {
            pSeed->growTime = 3600;
        }
        else
        {
            pSeed->growTime = (pSeed->rarity * pSeed->rarity * pSeed->rarity) + (30 * pSeed->rarity);
        }

        if(pSeed->growTime == 0) 
        {
            pSeed->growTime = 31;
        }
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

ItemInfoManager *GetItemInfoManager() { return ItemInfoManager::GetInstance(); }
