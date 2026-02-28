#include "ItemInfo.h"
#include "../Proton/ProtonUtils.h"

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
        memBuffer.ReadWrite(fxFlags, write);

        if(write) {
            memBuffer.WriteRaw(clientData, sizeof(clientData));
        }
        else {
            memBuffer.ReadRaw(clientData, sizeof(clientData));
        }
    }

    if(version > 9) {
        memBuffer.ReadWrite(tileRange, write);
        memBuffer.ReadWrite(pileSize, write);
    }

    if(version > 10) {
        memBuffer.ReadWriteString(customizedPunchParameters, write);
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

bool IsIllegalItem(uint32 itemID)
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
