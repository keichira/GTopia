#include "TileInfo.h"
#include "../Item/ItemInfoManager.h"
#include "../IO/Log.h"

TileInfo::TileInfo()
: m_pExtraData(nullptr), m_fg(0), m_bg(0), m_parent(0), m_flags(0)
{
}

TileInfo::~TileInfo()
{
    SAFE_DELETE(m_pExtraData)
}

void TileInfo::Serialize(MemoryBuffer& memBuffer, bool write, bool database)
{
    memBuffer.ReadWrite(m_fg, write);
    memBuffer.ReadWrite(m_bg, write);
    memBuffer.ReadWrite(m_parent, write);
    memBuffer.ReadWrite(m_flags, write);

    /*if(HasFlag(TILE_FLAG_HAS_PARENT)) {
        memBuffer.Seek(memBuffer.GetOffset() + 2);
    }

    if(HasFlag(TILE_FLAG_HAS_EXTRA_DATA)) {
        LOGGER_LOG_ERROR("%d %d %d %d", m_fg, m_bg, m_parent, m_flags)
        ItemInfo* pItem = GetItemInfoManager()->GetItemByID(m_fg);
        if(!pItem) {
            return;
        }

        m_pExtraData = new TileExtra();
        if(m_pExtraData->Setup(pItem->type)) {
            m_pExtraData->Serialize(memBuffer, write, database, this);
        }
    }*/
}

void TileInfo::SetFG(uint16 itemID)
{
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if(!pItem) {
        return;
    }

    /*if(m_pExtraData) {
        SAFE_DELETE(m_pExtraData);
    }

    if(itemID == 0) {
        
    }*/

    /*if(TileExtra::HasExtra(itemID)) {
        m_pExtraData = new TileExtra();
        m_pExtraData->Setup(pItem->type);

        SetFlag(TILE_FLAG_HAS_EXTRA_DATA);
    }*/

    m_fg = itemID;
}

void TileInfo::SetBG(uint16 itemID)
{
    /*ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if(!pItem || !pItem->IsBackground()) {
        return;
    }*/

    m_bg = itemID;
}
