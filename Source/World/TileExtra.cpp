#include "TileExtra.h"
#include "TileInfo.h"
#include "Item/ItemInfo.h"

uint8 GetTileExtraType(uint8 itemType)
{
    switch(itemType) {
        case ITEM_TYPE_USER_DOOR: case ITEM_TYPE_DOOR:
        case ITEM_TYPE_PORTAL: case ITEM_TYPE_SUNGATE:
            return TILE_EXTRA_TYPE_DOOR;

        case ITEM_TYPE_SIGN:
            return TILE_EXTRA_TYPE_SIGN;

        case ITEM_TYPE_LOCK:
            return TILE_EXTRA_TYPE_LOCK;

        case ITEM_TYPE_SEED:
            return TILE_EXTRA_TYPE_SEED;

        case ITEM_TYPE_PROVIDER:
            return TILE_EXTRA_TYPE_PROVIDER;

        default:
            return TILE_EXTRA_TYPE_NONE;
    }
}

TileExtra* CreateTileExtra(uint8 type)
{
    switch(type)
    {
        case TILE_EXTRA_TYPE_DOOR:
            return new TileExtra_Door();

        case TILE_EXTRA_TYPE_SIGN:
            return new TileExtra_Sign();

        case TILE_EXTRA_TYPE_LOCK:
            return new TileExtra_Lock();

        case TILE_EXTRA_TYPE_SEED:
            return new TileExtra_Seed();

        case TILE_EXTRA_TYPE_COMPONENT:
            return new TileExtra_Component();

        case TILE_EXTRA_TYPE_PROVIDER:
            return new TileExtra_Provider();

        case TILE_EXTRA_TYPE_LAB:
            return new TileExtra_Lab();

        case TILE_EXTRA_TYPE_HEART_MONITOR:
            return new TileExtra_HeartMonitor();

        default:
            return nullptr;
    }
}

void TileExtraFinalizeGrowth(TileExtra* pTileExtra, uint32& timer, uint32& growth, uint32 ageMS)
{
    if(!pTileExtra)
        return;

    uint32 now = Time::GetSystemTime();

    uint32 elapsedMS = now - timer;
    uint32 elapsedSec = elapsedMS / 1000;
    uint32 correctedTimer = now - (elapsedMS % 1000);

    if(ageMS != 0)
    {
        elapsedSec = ageMS / 1000;
        correctedTimer = timer;
    }

    if(pTileExtra->type == TILE_EXTRA_TYPE_TAMAGOTCHI)
    {
        return;
    }

    if(pTileExtra->type == TILE_EXTRA_TYPE_FORGE || pTileExtra->type == TILE_EXTRA_TYPE_STEAM_ENGINE || pTileExtra->type == TILE_EXTRA_TYPE_FOSSIL_PREP)
    {
        growth = (growth > elapsedSec) ? (growth - elapsedSec) : 0;
        timer = correctedTimer;
        return;
    }

    growth += elapsedSec;
    timer = correctedTimer;

    if(pTileExtra->type == TILE_EXTRA_TYPE_BULLETIN && growth != 0 /* && X != 0*/)
    {
        /**
         * 
         */

        growth = 0;
    }
    
}

void TileExtraModGrowth(TileExtra* pTileExtra, uint32& timer, uint32& growth, int32 deltaAgeSec, int32 ageSec)
{
    if(!pTileExtra)
        return;

    TileExtraFinalizeGrowth(pTileExtra, timer, growth, 0);

    if(deltaAgeSec < 1)
    {
        if(pTileExtra->type == TILE_EXTRA_TYPE_BURGLAR || pTileExtra->type == TILE_EXTRA_TYPE_STEAM_ENGINE || pTileExtra->type == TILE_EXTRA_TYPE_FOSSIL_PREP)
        {
            growth -= deltaAgeSec;

            if(growth > ageSec)
                growth = ageSec;
            return;
        }

        if(growth + deltaAgeSec < 0)
        {
            growth = 0;
            return;
        }

        growth += deltaAgeSec;
    }
    else 
    {
        TileExtraFinalizeGrowth(pTileExtra, timer, growth, deltaAgeSec * 1000);
    }

    if(growth > ageSec)
        growth = ageSec;
}

uint32 GetTileExtraGrowth(TileExtra* pTileExtra, uint32& timer, uint32& growth)
{
    TileExtraFinalizeGrowth(pTileExtra, timer, growth, 0);

    return growth;
}

float GetTileExtraGrowthPercent(uint32 requiredTime, uint32 growth)
{
    if(requiredTime == 0.0f)
    {
        requiredTime = 0.0001f;
    }

    float progress = (float)growth / requiredTime;

    if(progress > 1.0f)
    {
        progress = 1.0f;
    }

    return progress * 100.0f;
}

void TileExtra::Serialize(MemoryBuffer& memBuffer, bool write)
{
    memBuffer.ReadWrite(type, write);
}

void TileExtra_Door::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile, uint16 worldVersion)
{
    TileExtra::Serialize(memBuffer, write);

    if(IsMainDoor(pTile->GetFG())) 
    {
        name = "EXIT";
    }

    memBuffer.ReadWriteString(name, write);

    if(database) {
        memBuffer.ReadWriteString(text, write);
        memBuffer.ReadWriteString(id, write);
    }

    int8 unk = 0;
    memBuffer.ReadWrite(unk, write);
}

void TileExtra_Sign::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile, uint16 worldVersion)
{
    TileExtra::Serialize(memBuffer, write);

    memBuffer.ReadWriteString(text, write);

    int32 unk = -1; // something with owner union but eh
    memBuffer.ReadWrite(unk, write);
}

void TileExtra_Lock::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo *pTile, uint16 worldVersion)
{
    TileExtra::Serialize(memBuffer, write);

    memBuffer.ReadWrite(flags, write);
    memBuffer.ReadWrite(ownerID, write);

    uint32 accessSize = accessList.size();
    memBuffer.ReadWrite(accessSize, write);

    if(!write) {
        accessList.resize(accessSize);
    }

    if(accessSize > 0) {
        memBuffer.ReadWriteRaw(accessList.data(), accessSize * sizeof(int32), write);
    }

    if(worldVersion > 11) {
        memBuffer.ReadWrite(minEntryLevel, write);
    }

    if(worldVersion > 12) {
        memBuffer.ReadWrite(worldTimer, write);
    }
}

void TileExtra_Seed::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile, uint16 worldVersion)
{
    TileExtraFinalizeGrowth(this, timer, growTime, 0);

    TileExtra::Serialize(memBuffer, write);
    memBuffer.ReadWrite(growTime, write);
    memBuffer.ReadWrite(fruitCount, write);
}

void TileExtra_Component::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile, uint16 worldVersion)
{
    TileExtra::Serialize(memBuffer, write);
    memBuffer.ReadWrite(randValue, write);
}

void TileExtra_Provider::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile, uint16 worldVersion)
{
    TileExtraFinalizeGrowth(this, timer, growTime, 0);

    TileExtra::Serialize(memBuffer, write);
    memBuffer.ReadWrite(growTime, write);

    /*uint16 fgItem = pTile->GetFG();
    if(fgItem == ITEM_ID_WINTERFEST_CALENDAR_2017) {
        memBuffer.ReadWrite(otherData, write);
    }

    if(!database && (fgItem == ITEM_ID_WINTERFEST_CALENDAR_2018 || fgItem == ITEM_ID_WINTERFEST_CALENDAR_2019)) {
        memBuffer.ReadWrite(otherData, write);
    }

    if(!database && fgItem == ITEM_ID_BUILDING_BLOCKS_MACHINE) {
        // long
        // long
    }

    if(worldVersion > 9 &&
        (
            fgItem == ITEM_ID_KEENAN_GTS_AWESOME_ITEM_O_MATIC ||
            fgItem == ITEM_ID_VENOMSTS_AWESOME_ITEM_O_MATIC ||
            fgItem == ITEM_ID_LINKTRADER_GTS_AWESOME_ITEM_O_MATIC ||
            fgItem == ITEM_ID_TERYS_AWESOME_ITEM_O_MATIC ||
            fgItem == ITEM_ID_EVETS_GTS_AWESOME_ITEM_O_MATIC ||
            fgItem == ITEM_ID_BENBARRAGES_AWESOME_ITEM_O_MATIC ||
            fgItem == ITEM_ID_MRSONGO_GTS_AWESOME_ITEM_O_MATIC ||
            fgItem == ITEM_ID_OLDGERTIES_AWESOME_ITEM_O_MATIC ||
            fgItem == ITEM_ID_MACPROOF_GTS_AWESOME_ITEM_O_MATIC
        ))
    {
        // long
        // long
        // same as building blocks machine
    }*/
}

void TileExtra_Lab::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo *pTile, uint16 worldVersion)
{
    TileExtra::Serialize(memBuffer, write);
    memBuffer.ReadWrite(userID, write);
    memBuffer.ReadWrite(achievementID, write);
}

void TileExtra_HeartMonitor::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo *pTile, uint16 worldVersion)
{
    TileExtra::Serialize(memBuffer, write);
    memBuffer.ReadWrite(userID, write);
    memBuffer.ReadWrite(playerDisplayName, write);
}
