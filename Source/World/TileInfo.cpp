#include "TileInfo.h"
#include "../Item/ItemInfoManager.h"
#include "../IO/Log.h"
#include "WorldTileManager.h"

TileInfo::TileInfo()
: m_pExtraData(nullptr), m_fg(0), m_bg(0), m_parent(0), m_flags(0), m_damage(0)
{
}

TileInfo::~TileInfo()
{
    SAFE_DELETE(m_pExtraData)
}

void TileInfo::Serialize(MemoryBuffer& memBuffer, bool write, bool database, uint16 worldVersion)
{
    memBuffer.ReadWrite(m_fg, write);
    memBuffer.ReadWrite(m_bg, write);
    memBuffer.ReadWrite(m_parent, write);
    memBuffer.ReadWrite(m_flags, write);

    if(HasFlag(TILE_FLAG_HAS_PARENT)) {
        memBuffer.Seek(memBuffer.GetOffset() + 2);
    }

    if(HasFlag(TILE_FLAG_HAS_EXTRA_DATA)) {
        ItemInfo* pItem = GetItemInfoManager()->GetItemByID(m_fg);
        if(!pItem) {
            return;
        }

        if(!write) {
            m_pExtraData = TileExtra::Create(GetTileExtraType(pItem->type));
            if(!m_pExtraData) {
                LOGGER_LOG_ERROR("Tile flagged with extra data but extra data is NULL? fg:%d itemType:%d", m_fg, pItem->type);
                return;
            }
            m_pExtraData->Serialize(memBuffer, write, database, this, worldVersion);
        }
        else {
            if(!m_pExtraData) {
                LOGGER_LOG_ERROR("Tile flagged with extra data but extra data is NULL? fg:%d itemType:%d", m_fg, pItem->type);
                return;
            }
            m_pExtraData->Serialize(memBuffer, write, database, this, worldVersion);
        }
    }
}

void TileInfo::SetFG(uint16 itemID, WorldTileManager* pTileMgr)
{
    if(!pTileMgr) {
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if(!pItem) {
        return;
    }

    if(m_pExtraData) {
        SAFE_DELETE(m_pExtraData);
        RemoveFlag(TILE_FLAG_HAS_EXTRA_DATA);
    }

    if(itemID == ITEM_ID_BLANK) {
        m_lastDamageTime.Reset();
        m_damage = 0;

        pTileMgr->ModifyKeyTile(this, true);
        m_fg = itemID;
        return;
    }

    uint8 tileExtraType = GetTileExtraType(pItem->type);
    if(tileExtraType != TILE_EXTRA_TYPE_NONE) {
        SetFlag(TILE_FLAG_HAS_EXTRA_DATA);
        
        m_pExtraData = TileExtra::Create(tileExtraType);
    }

    m_fg = itemID;
    pTileMgr->ModifyKeyTile(this, false);
}

void TileInfo::SetBG(uint16 itemID)
{
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if(!pItem) {
        return;
    }

    if(!pItem->IsBackground() && itemID != ITEM_ID_BLANK) {
        return;
    }

    if(itemID == ITEM_ID_BLANK) {
        m_lastDamageTime.Reset();
        m_damage = 0;
    }

    m_bg = itemID;
}

void TileInfo::PunchTile(uint8 damage)
{
    uint16 itemToDamage = GetDisplayedItem();

    if(itemToDamage == ITEM_ID_BLANK) {
        LOGGER_LOG_WARN("Mate we are damaging nothingness?");
        return;
    }
    
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemToDamage);
    if(!pItem) {
        return;
    }

    if(m_lastDamageTime.GetElapsedTime() >= pItem->restoreTime * 1000) {
        m_lastDamageTime.Reset();
        m_damage = 0;
    }

    if(m_damage + damage < pItem->hp) {
        m_damage += damage;
    }
    else {
        m_damage = pItem->hp;
    }

    m_lastDamageTime.Reset();

    if(
        pItem->type == ITEM_TYPE_RACE_FLAG || pItem->type == ITEM_TYPE_SWITCHEROO ||
        (pItem->type == ITEM_TYPE_DEADLY_IF_ON && pItem->id != ITEM_ID_STEAM_SPIKES)
    ) {
        ToggleFlag(TILE_FLAG_IS_ON);
    }
}

float TileInfo::GetHealthPercent()
{
    uint16 itemToDamage = GetDisplayedItem();

    if(itemToDamage == ITEM_ID_BLANK) {
        return 1.0f;
    }
    
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemToDamage);
    if(!pItem) {
        return 1.0f;
    }

    if(m_lastDamageTime.GetElapsedTime() >= pItem->restoreTime * 1000) {
        m_lastDamageTime.Reset();
        m_damage = 0;

        return 1.0f;
    }
    
    return 1.0f - (m_damage / pItem->hp);
}

uint16 TileInfo::GetDisplayedItem()
{
    return m_fg != ITEM_ID_BLANK ? m_fg : m_bg;
}