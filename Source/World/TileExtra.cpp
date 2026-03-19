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

        default:
            return nullptr;
    }
}

void TileExtra::Serialize(MemoryBuffer &memBuffer, bool write)
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

    uint8 unk = 0;
    memBuffer.ReadWrite(unk, write);
}

void TileExtra_Sign::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile, uint16 worldVersion)
{
    TileExtra::Serialize(memBuffer, write);

    memBuffer.ReadWriteString(text, write);

    int32 unk = -1; // something with owner union but eh
    memBuffer.ReadWrite(unk, write);
}


/**
 * 
 * memBuffer.ReadWrite(m_flags, write); // u8
    memBuffer.ReadWrite(m_ownerID, write);

    uint32 extraEntrySize = m_extraEntries.size();
    memBuffer.ReadWrite(extraEntrySize, write);

    if(!write) {
        m_extraEntries.resize(extraEntrySize);
    }

    if(extraEntrySize > 0) {
    
        memBuffer.ReadWriteRaw(m_extraEntries.data(), extraEntrySize * sizeof(int32), write);
    }

    if(worldVersion > 11) {
        memBuffer.ReadWrite(m_minEntryLevel, write);
    }

    if(worldVersion > 12) {
        memBuffer.ReadWrite(m_worldTimer, write);
    }
 * 
 */