#include "TileInfo.h"
#include "../Item/ItemInfoManager.h"
#include "../IO/Log.h"
#include "WorldTileManager.h"
#include "WorldInfo.h"

TileInfo::TileInfo()
: m_pExtraData(nullptr), m_tileData(nullptr), m_damage(0)
{
}

TileInfo::~TileInfo()
{
    SAFE_DELETE(m_pExtraData)
}

void TileInfo::Serialize(MemoryBuffer& memBuffer, bool write, bool database, WorldInfo* pWorld)
{
    if(!database) {
        memBuffer.ReadWrite(m_tileData->fg, write);
        memBuffer.ReadWrite(m_tileData->bg, write);
        memBuffer.ReadWrite(m_tileData->parent, write);
        memBuffer.ReadWrite(m_tileData->flags, write);
    }

    if(HasFlag(TILE_FLAG_HAS_PARENT)) {
        memBuffer.ReadWrite(m_tileData->parent, write); // ye its like that
    }

    if(HasFlag(TILE_FLAG_HAS_EXTRA_DATA)) {
        if(write) {
            if(!m_pExtraData) {
                LOGGER_LOG_ERROR("Tile flagged with extra data but extra data is NULL? fg:%d", m_tileData->fg);
                return;
            }
            
            m_pExtraData->Serialize(memBuffer, true, database, this, pWorld->GetWorldVersion());
        }
        else {
            ItemInfo* pItem = GetItemInfoManager()->GetItemByID(m_tileData->fg);
            if(!pItem) {
                return;
            }

            uint8 tileExtraType = GetTileExtraType(pItem->type);
            if(tileExtraType != TILE_EXTRA_TYPE_NONE) {           
                m_pExtraData = CreateTileExtra(tileExtraType);     

                if(m_pExtraData) {
                    m_pExtraData->Serialize(memBuffer, false, database, this, pWorld->GetWorldVersion());
                }
            }
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

    if(itemID == ITEM_ID_BLANK) 
    {
        m_lastDamageTime.Reset();
        m_damage = 0;

        pTileMgr->ModifyKeyTile(this, true);
        m_tileData->fg = itemID;

        RemoveFlag(TILE_FLAG_WAS_SPLICED);
        RemoveFlag(TILE_FLAG_IS_ON);
        RemoveFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC);
        RemoveFlag(TILE_FLAG_FG_ALT_MODE);
        return;
    }

    uint8 tileExtraType = GetTileExtraType(pItem->type);
    if(tileExtraType != TILE_EXTRA_TYPE_NONE) {
        SetFlag(TILE_FLAG_HAS_EXTRA_DATA);
        
        m_pExtraData = CreateTileExtra(tileExtraType);
    }

    m_tileData->fg = itemID;
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

    m_tileData->bg = itemID;
}

bool TileInfo::IsCollidable()
{
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(GetDisplayedItem());
    if(!pItem)
        return true;

    if(pItem->collisionType == COLLISION_IF_OFF)
        return !HasFlag(TILE_FLAG_IS_ON);

    if(pItem->collisionType == COLLISION_IF_ON)
        return HasFlag(TILE_FLAG_IS_ON);

    return !(pItem->collisionType == COLLISION_NONE || pItem->collisionType == COLLISION_ONE_WAY);
}

void TileInfo::BindTileData(TempTileData* pTileData)
{
    m_tileData = pTileData;
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
        pItem->type == ITEM_TYPE_CHEST || pItem->type == ITEM_TYPE_SWITCHEROO ||
        (pItem->type == ITEM_TYPE_DEADLY_IF_ON && pItem->id != ITEM_ID_STEAM_SPIKES) ||
        pItem->type == ITEM_TYPE_BOOMBOX || pItem->type == ITEM_TYPE_BOOMBOX2
    ) {
        ToggleFlag(TILE_FLAG_IS_ON);
    }
}

bool TileInfo::WillBreak(uint8 damage)
{
    uint16 itemToDamage = GetDisplayedItem();

    if(itemToDamage == ITEM_ID_BLANK)
        return false;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemToDamage);
    if(!pItem)
        return false;

    if(m_lastDamageTime.GetElapsedTime() >= pItem->restoreTime * 1000)
        return false;

    return m_damage + damage >= pItem->hp;
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
    
    return 1.0f - ((float)m_damage / pItem->hp);
}

bool TileInfo::IsTree()
{
    if(m_tileData->fg == ITEM_ID_BLANK)
        return false;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(m_tileData->fg);
    if(!pItem)
        return false;

    return pItem->type == ITEM_TYPE_SEED;
}

bool TileInfo::IsFlammable()
{
    if(HasFlag(TILE_FLAG_ON_FIRE) || HasFlag(TILE_FLAG_IS_WET))
        return false;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(GetDisplayedItem());
    if(!pItem)
        return false;

    if(pItem->id == ITEM_ID_BLANK)
        return false;

    if(pItem->type == ITEM_TYPE_DOOR || pItem->type == ITEM_TYPE_USER_DOOR || pItem->type == ITEM_TYPE_BEDROCK || pItem->type == ITEM_TYPE_PORTAL ||
        pItem->type == ITEM_TYPE_CHECKPOINT || pItem->type == ITEM_TYPE_SUNGATE || pItem->type == ITEM_TYPE_LOCK ||
        pItem->type == ITEM_TYPE_TEAM || pItem->type == ITEM_TYPE_ADVENTURE_RESET
    ) {
        return false;
    }

    if(pItem->id == ITEM_ID_ULTRA_PINATA)
        return false;

    return true;
}

float TileInfo::GetGrowthPercent()
{
    if(!m_pExtraData)
        return 0.0f;

    return m_pExtraData->GetGrowthPercent(this);
}

void TileInfo::FinalizeGrowth(uint32 ageMS)
{
    if(!m_pExtraData)
        return;

    m_pExtraData->FinalizeGrowth(ageMS);
}

void TileInfo::ModGrowth(int32 deltaAgeSec, int32 ageSec)
{
    if(!m_pExtraData)
        return;

    m_pExtraData->ModGrowth(deltaAgeSec, ageSec);
}

uint16 TileInfo::GetDisplayedItem()
{
    return m_tileData->fg != ITEM_ID_BLANK ? m_tileData->fg : m_tileData->bg;
}

void TileInfo::AgeTile(uint32 ageMS)
{
    if(GetDisplayedItem() == ITEM_ID_BLANK)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(GetDisplayedItem());
    if(!pItem)
        return;

    ModGrowth(ageMS/1000, pItem->growTime);
}
