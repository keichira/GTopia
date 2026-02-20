#include "TileInfo.h"
#include "../Item/ItemInfoManager.h"

TileInfo::TileInfo()
: m_pExtraData(nullptr)
{
}

TileInfo::~TileInfo()
{
    SAFE_DELETE(m_pExtraData)
}

void TileInfo::Serialize(MemoryBuffer &memBuffer, bool write, bool database)
{
    memBuffer.ReadWrite(m_fg, write);
    memBuffer.ReadWrite(m_bg, write);
    memBuffer.ReadWrite(m_parent, write);
    memBuffer.ReadWrite(m_flags, write);

    if(HasFlag(TILE_FLAG_HAS_PARENT)) {
        memBuffer.ReadWrite(m_parent, write);
    }

    if(HasFlag(TILE_FLAG_HAS_EXTRA_DATA)) {
        ItemInfo* pItem = GetItemInfoManager()->GetItemByID(m_fg);
        if(!pItem) {
            return;
        }

        m_pExtraData = new TileExtra();
        if(m_pExtraData->Setup(pItem->type)) {
            m_pExtraData->Serialize(memBuffer, write, database, this);
        }
    }
}

void TileInfo::SetFG(uint16 itemID)
{
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if(!pItem) {
        return;
    }

    if(m_pExtraData) {
        SAFE_DELETE(m_pExtraData);
    }

    if(itemID == 0) {
        
    }

    if(TileExtra::HasExtra(itemID)) {
        m_pExtraData = new TileExtra();
        m_pExtraData->Setup(pItem->type);

        SetFlag(TILE_FLAG_HAS_EXTRA_DATA);
    }
}

void TileInfo::SetBG(uint16 itemID)
{
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if(!pItem || !pItem->IsBackground()) {
        return;
    }

    m_bg = itemID;
}
