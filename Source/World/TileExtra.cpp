#include "TileExtra.h"
#include "TileInfo.h"
#include "Item/ItemUtils.h"

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

        default:
            return TILE_EXTRA_TYPE_NONE;
    }
}

TileExtra* TileExtra::Create(uint8 tileExtraType)
{
    switch(tileExtraType) {
        case TILE_EXTRA_TYPE_DOOR:
            return new TileExtra_Door();

        case TILE_EXTRA_TYPE_SIGN:
            return new TileExtra_Sign();

        case TILE_EXTRA_TYPE_LOCK:
            return new TileExtra_Lock(); 

        default:
            return nullptr;
    }
}

void TileExtra::Serialize(MemoryBuffer& memBuffer, bool write)
{
    memBuffer.ReadWrite(type, write);
}

void TileExtra_Door::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile, uint16 worldVersion)
{
    TileExtra::Serialize(memBuffer, write);
    memBuffer.ReadWriteString(name, write);

    if(database) {
        memBuffer.ReadWriteString(text, write);
        memBuffer.ReadWriteString(id, write);
    }

    int8 unk = -1;
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
