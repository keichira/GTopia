#include "ItemInfo.h"
#include "../Proton/ProtonUtils.h"
#include "../Math/Random.h"
#include "ItemInfoManager.h"

ItemInfo::ItemInfo()
{
}

void ItemInfo::Serialize(MemoryBuffer& memBuffer, bool write, uint16 version)
{
    memBuffer.ReadWrite(id, write);
    memBuffer.ReadWrite(flags, write);
    memBuffer.ReadWrite(type, write);
    memBuffer.ReadWrite(material, write);

    if(version < 3) {
        memBuffer.ReadWriteString(name, write);
    }
    else {
        if(write) {
            string writeName = XorCipherString(name, "PBG892FXX982ABC*", id);
            memBuffer.WriteStringRaw(writeName);
        }
        else {
            memBuffer.ReadStringRaw(name);
            name = XorCipherString(name, "PBG892FXX982ABC*", id);
        }
    }

    memBuffer.ReadWriteString(textureFile, write);
    memBuffer.ReadWrite(textureHash, write);
    memBuffer.ReadWrite(visualEffect, write);
    memBuffer.ReadWrite(cookingTime, write);
    memBuffer.ReadWrite(textureX, write);
    memBuffer.ReadWrite(textureY, write);
    memBuffer.ReadWrite(storage, write);
    memBuffer.ReadWrite(layer, write);
    memBuffer.ReadWrite(collisionType, write);
    memBuffer.ReadWrite(hp, write);
    memBuffer.ReadWrite(restoreTime, write);
    memBuffer.ReadWrite(bodyPart, write);
    memBuffer.ReadWrite(rarity, write);
    memBuffer.ReadWrite(maxCanHold, write);
    memBuffer.ReadWriteString(extraString, write);
    memBuffer.ReadWrite(extraStringHash, write);
    memBuffer.ReadWrite(animMS, write);

    if(version > 3) {
        memBuffer.ReadWriteString(petName, write);
        memBuffer.ReadWriteString(petSubName, write);
        memBuffer.ReadWriteString(petEndName, write);
    }

    if(version > 4) {
        memBuffer.ReadWriteString(petPowerName, write);
    }

    memBuffer.ReadWrite(seedBg, write);
    memBuffer.ReadWrite(seedFg, write);
    memBuffer.ReadWrite(treeBg, write);
    memBuffer.ReadWrite(treeFg, write);

    memBuffer.ReadWrite(seedBgColor, write);
    memBuffer.ReadWrite(seedFgColor, write);

    uint16 temp = 0;
    memBuffer.ReadWrite(temp, write);
    memBuffer.ReadWrite(temp, write);

    memBuffer.ReadWrite(growTime, write);

    if(version > 6) {
        memBuffer.ReadWrite(fxFlags, write);
        memBuffer.ReadWriteString(multiAnim1, write);
    }

    if(version > 7) {
        memBuffer.ReadWriteString(overlayTextureFile, write);
        memBuffer.ReadWriteString(multiAnim2, write);
        memBuffer.ReadWrite(dualAnimLayer, write);
    }

    if(version > 8) {
        memBuffer.ReadWrite(flags2, write);
        memBuffer.ReadWriteRaw(clientData, sizeof(clientData), write);
    }

    if(version > 9) {
        memBuffer.ReadWrite(tileRange, write);
        memBuffer.ReadWrite(pileSize, write);
    }

    if(version > 10) {
        memBuffer.ReadWriteString(customizedPunchParameters, write);
    }

    if(version > 11) {
        memBuffer.ReadWrite(extraSlotCounter, write);
        memBuffer.ReadWriteRaw(extraSlotBodyParts, sizeof(extraSlotBodyParts), write);
    }

    if(version > 12) {
        memBuffer.ReadWrite(lightSourceRange, write);
    }

    if(version > 13) {
        memBuffer.ReadWrite(variantVersionItem, write);
    }

    if(version > 14) {
        memBuffer.ReadWrite(chairInfo.enabled, write);
        memBuffer.ReadWrite(chairInfo.playerOffset, write);
        memBuffer.ReadWrite(chairInfo.armPos, write);
        memBuffer.ReadWrite(chairInfo.armOffset, write);
        memBuffer.ReadWriteString(chairInfo.armTexture, write);
    }

    if(version > 15) {
        memBuffer.ReadWriteString(configName, write);
    }

    if(version > 16) {
        memBuffer.ReadWrite(otherPlayerHitParticle, write);
    }

    if(version > 17) {
        memBuffer.ReadWrite(configNameHash, write);
    }

    if(version > 18) {
        memBuffer.ReadWrite(randomSpriteInfo.enabled, write);
        memBuffer.ReadWrite(randomSpriteInfo.offsetMod, write);
        memBuffer.ReadWrite(randomSpriteInfo.chance, write);
    }

    if(version > 19) {
        // HEAD, FACE, BODY, FRONT_ARM, BACK_ARM, LEGS
        memBuffer.ReadWrite(hiddenPartsFlags, write);
    }

    if(version > 20) {
        memBuffer.ReadWrite(canTransform, write);
    }

    if(version > 21) {
        memBuffer.ReadWriteString(description, write);
    }

    if(version > 22) {
        memBuffer.ReadWrite(seed1, write);
        memBuffer.ReadWrite(seed2, write);
    }

    if(version > 23) {
        // NONE, SLIP, NO_SLIP
        memBuffer.ReadWrite(slipperyType, write);
    }

    if(version > 24) {
        string unk;
        memBuffer.ReadWriteString(unk, write);

        uint32 unk2 = 0;
        memBuffer.ReadWrite(unk2, write);
    }

    if(version > 25) {
        uint8 unk = 0;
        memBuffer.ReadWrite(unk, write);
    }
}

bool ItemInfo::IsBackground()
{
    if(
        type == ITEM_TYPE_BACKGROUND || type == ITEM_TYPE_BACKGD_SFX_EXTRA_FRAME ||
        type == ITEM_TYPE_BACK_BOOMBOX || type == ITEM_TYPE_MUSICNOTE
    ) {
        return true;
    }

    return false;
}

bool IsIllegalItem(uint16 itemID)
{
    switch(itemID) {
        case ITEM_ID_BLANK_SEED:
        case ITEM_ID_MAIN_DOOR:
        case ITEM_ID_MAIN_DOOR_SEED:
        case ITEM_ID_FIST_SEED:
        case ITEM_ID_WRENCH_SEED:
            return true;

        default:
            return false;
    }
}

bool IsWorldLock(uint16 itemID)
{
    switch(itemID) {
        case ITEM_ID_WORLD_LOCK:
        case ITEM_ID_DIAMOND_LOCK:
        case ITEM_ID_HARMONIC_LOCK:
        case ITEM_ID_ROBOTIC_LOCK:
            return true;

        default:
            return false;
    }
}

bool IsMainDoor(uint16 itemID)
{
    switch(itemID) {
        case ITEM_ID_MAIN_DOOR:
            return true;

        default:
            return false;
    }
}

bool IsFuelPack(uint16 itemID)
{
    switch(itemID) {
        case ITEM_ID_FUEL_PACK:
        case ITEM_ID_ECTO_PACK:
        case ITEM_ID_PINEAPPLE_JUICE:
        case ITEM_ID_DANGEROUS_PINEAPPLE_JUICE_BACKPACK:
            return true;

        default:
            return false;
    }
}

uint16 GetMaxTilesToLock(uint16 itemID)
{
    switch(itemID) {
        case ITEM_ID_SMALL_LOCK:
            return 10;

        case ITEM_ID_BIG_LOCK:
            return 48;

        case ITEM_ID_HUGE_LOCK:
            return 200;

        default:
            return 0;
    }
}

void GetTreeSpawnInfo(ItemInfo* pItem, uint32& fruitCount, bool& dropSeed)
{
    fruitCount = 0;
    dropSeed = false;

    if(!pItem)
        return;

    switch(pItem->id)
    {
        case ITEM_ID_LEGENDARY_WIZARD_SEED:
        case ITEM_ID_WIZARDS_STAFF:
        case ITEM_ID_SHIFTING_SCROLL:
        {
            fruitCount = 1;
            break;
        }

        case ITEM_ID_LEGEN_SEED:
        case ITEM_ID_DARY_SEED:
        case ITEM_ID_MUTATED_SEED:
        {
            fruitCount = 0;
            break;
        }

        default:
        {
            fruitCount = RandomRangeInt(1, pItem->maxFruitCount - 1);
        }
    }
    
    if(!pItem->HasFlag(ITEM_FLAG_SEEDLESS))
    {
        dropSeed = (RandomRangeInt(0, pItem->rarity/4 + 4) == 0);
    }
}

uint32 GetGemCountHarvestTree(ItemInfo* pSeed)
{
    if(!pSeed)
        return 0;

    if(pSeed->rarity == 999 || pSeed->HasFlag2(ITEM_FLAG_GEMLESS))
        return 0;
    
    if(pSeed->type == ITEM_TYPE_SEED)
    {
        ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pSeed->id - 1);
        if(pItem && pItem->HasFlag2(ITEM_FLAG_GEMLESS))
            return 0;
    }

    uint32 value = pSeed->rarity / 4;
    if(pSeed->rarity < 31)
    {
        value = (value * 3) / 4;
    }

    if(value < 2)
        value = 2;

    return RandomRangeInt(0, value - 1);
}

void GetBlockSpawnInfo(ItemInfo* pItem, bool isLucky, bool& dropBlock, bool& dropSeed, int32& dropGems)
{
    dropBlock = false;
    dropSeed = false;
    dropGems = 0;

    if(!pItem)
        return;

    if(RandomRangeInt(0, 2) == 0 || isLucky)
    {
        if(!pItem->HasFlag(ITEM_FLAG_DROPLESS))
        {
            dropBlock = true;
        }

        if(isLucky)
        {
            if(RandomRangeInt(0, 3) == 0 && !pItem->HasFlag(ITEM_FLAG_SEEDLESS))
            {
                dropSeed = true;
            }
        }
        else if(RandomRangeInt(0, 3) != 0)
        {
            if(!pItem->HasFlag(ITEM_FLAG_SEEDLESS))
            {
                dropSeed = true;
            }
            dropBlock = false;
        }

        if(!isLucky)
            return;
    }

    if(pItem->rarity != 999 && !pItem->HasFlag(ITEM_FLAG_SEEDLESS) && !pItem->HasFlag2(ITEM_FLAG_GEMLESS))
    {
        int32 randGem = RandomRangeInt(0, 19);
        if(randGem == 0 || isLucky)
        {
            dropGems = 1;
        }

        int32 value = pItem->rarity / 4;
        if(value > 1)
        {
            if(pItem->rarity < 31)
            {
                value = (value * 3) / 4;
            }

            dropGems += RandomRangeInt(0, value - 1);

            if(isLucky)
            {
                dropGems *= 5;
            }
        }
    }
}
