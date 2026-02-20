#include "TileExtra.h"
#include "../Item/ItemUtils.h"
#include "TileInfo.h"

TileExtra::TileExtra()
{
}

bool TileExtra::HasExtra(uint8 itemType)
{
    switch(itemType) {
        case ITEM_TYPE_USER_DOOR: case ITEM_TYPE_DOOR:
        case ITEM_TYPE_PORTAL: case ITEM_TYPE_SUNGATE:
            return true;

        default:
            return false;
    }
}

bool TileExtra::Setup(uint8 itemType)
{
    switch(itemType) {
        case ITEM_TYPE_USER_DOOR: case ITEM_TYPE_DOOR:
        case ITEM_TYPE_PORTAL: case ITEM_TYPE_SUNGATE: {
            m_type = TILE_EXTRA_TYPE_DOOR;
            break;
        }

        default:
            return false;
    }

    return true;
}

bool TileExtra::Serialize(MemoryBuffer& memBuffer, bool write, bool database, TileInfo* pTile)
{
    if(!pTile || m_type == TILE_EXTRA_TYPE_NONE) {
        return false;
    }

    switch(m_type) {
        case TILE_EXTRA_TYPE_DOOR: {
            memBuffer.ReadWriteString<uint16>(m_name, write);

            if(database) {
                memBuffer.ReadWriteString<uint16>(m_text, write);
                memBuffer.ReadWriteString<uint16>(m_id, write);
            }

            uint8 unk = 0;
            memBuffer.ReadWrite(unk, write);
            break;
        }
    }

    return true;
}
