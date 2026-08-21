#include "SuckerBlockManager.h"
#include "Item/ItemInfoManager.h"
#include "World/WorldManager.h"

SuckerBlockManager::SuckerBlockManager(World* pWorld) : m_pWorld(pWorld) {}

bool SuckerBlockManager::IsCorrupted(TileExtra_Sucker* pSucker)
{
    if (!pSucker)
        return true;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pSucker->itemID);
    if (!pItem)
    {
        pSucker->itemID = 0;
        pSucker->count = 0;
        pSucker->isSucking = 1;
        pSucker->isPlanting = 0;
        return true;
    }

    return false;
}

bool SuckerBlockManager::IsAllowedToBuildOrPlantItem(int32 itemID)
{
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
        return false;

    if (pItem->HasFlag(ITEM_FLAG_PERMANENT) || pItem->HasFlag(ITEM_FLAG_AUTOPICKUP) ||
        pItem->HasFlag(ITEM_FLAG_PUBLIC) || pItem->HasFlag(ITEM_FLAG_UNTRADEABLE))
        return false;

    if (pItem->id == ITEM_ID_WATER_BUCKET || pItem->id == ITEM_ID_DIAMOND_HORN || pItem->id == ITEM_ID_DIAMOND_HORNS ||
        pItem->id == ITEM_ID_DIAMOND_DEVIL_HORNS)
        return false;

    switch (pItem->type)
    {
        case ITEM_TYPE_FIST:
        case ITEM_TYPE_WRENCH:
        case ITEM_TYPE_LOCK:
        case ITEM_TYPE_GEMS:
        case ITEM_TYPE_CONSUMABLE:
        case ITEM_TYPE_DOOR:
        case ITEM_TYPE_BEDROCK:
        case ITEM_TYPE_CLOTHES:
        case ITEM_TYPE_POINTY:
        case ITEM_TYPE_MAILBOX:
        case ITEM_TYPE_PINATA:
        case ITEM_TYPE_ACHIEVEMENT:
        case ITEM_TYPE_PROFILE:
        case ITEM_TYPE_MANNEQUIN:
        case ITEM_TYPE_TEAM:
        case ITEM_TYPE_SPOTLIGHT:
        case ITEM_TYPE_STEAMPUNK:
        case ITEM_TYPE_STEAM_ORGAN:
        case ITEM_TYPE_FLAG:
        case ITEM_TYPE_ARTCANVAS:
        case ITEM_TYPE_BATTLE_CAGE:
        case ITEM_TYPE_PET_TRAINER:
        case ITEM_TYPE_STEAM_ENGINE:
        case ITEM_TYPE_LOCK_BOT:
        case ITEM_TYPE_WEATHER_SPECIAL:
        case ITEM_TYPE_SPIRIT_STORAGE:
        case ITEM_TYPE_DISPLAY_SHELF:
        case ITEM_TYPE_VIP_DOOR:
        case ITEM_TYPE_CHAL_TIMER:
        case ITEM_TYPE_CHAL_FLAG:
        case ITEM_TYPE_PORTRAIT:
        case ITEM_TYPE_WEATHER_SPECIAL2:
        case ITEM_TYPE_FOSSIL:
        case ITEM_TYPE_FOSSIL_PREP:
        case ITEM_TYPE_DNA_MACHINE:
        case ITEM_TYPE_BLASTER:
        case ITEM_TYPE_VALHOWLA:
        case ITEM_TYPE_CHEMSYNTH:
        case ITEM_TYPE_CHEMTANK:
        case ITEM_TYPE_STORAGE:
        case ITEM_TYPE_OVEN:
        case ITEM_TYPE_TOMB_ROBBER:
        case ITEM_TYPE_TRAMPOLINE_MOMENTUM:
        case ITEM_TYPE_FISHGOTCHI_TANK:
        case ITEM_TYPE_SUCKER:
        case ITEM_TYPE_PLANTER:
        case ITEM_TYPE_ROBOT:
        case ITEM_TYPE_COMMAND:
        case ITEM_TYPE_LUCKY_TICKET:
        case ITEM_TYPE_STATS_BLOCK:
        case ITEM_TYPE_FIELD_NODE:
        case ITEM_TYPE_OUIJA_BOARD:
        case ITEM_TYPE_ARCHITECT_MACHINE:
        case ITEM_TYPE_STARSHIP:
        case ITEM_TYPE_AUTODELETE:
        case ITEM_TYPE_BOOMBOX2:
        case ITEM_TYPE_AUTO_ACTION_BREAK:
        case ITEM_TYPE_AUTO_ACTION_HARVEST:
        case ITEM_TYPE_AUTO_ACTION_HARVEST_SUCK:
        case ITEM_TYPE_LIGHTNING_CLOUD:
        case ITEM_TYPE_PHASED_BLOCK:
        case ITEM_TYPE_MUD:
        case ITEM_TYPE_ROOT_CUTTING:
        case ITEM_TYPE_PASSWORD_STORAGE:
        case ITEM_TYPE_PHASED_BLOCK_2:
        case ITEM_TYPE_BOMB:
        case ITEM_TYPE_PVE_NPC:
        case ITEM_TYPE_INFINITY_WEATHER_MACHINE:
        case ITEM_TYPE_SLIME:
        case ITEM_TYPE_ACID:
        case ITEM_TYPE_COMPLETIONIST:
        case ITEM_TYPE_PUNCH_TOGGLE:
        case ITEM_TYPE_ANZU_BLOCK:
        case ITEM_TYPE_FEEDING_BLOCK:
        case ITEM_TYPE_KRANKENS_BLOCK:
        case ITEM_TYPE_FRIENDS_ENTRANCE:
        case ITEM_TYPE_PEARLS:
            return false;

        default:
            return true;
    }
}

int32 SuckerBlockManager::GetMachineCapacity(int32 itemID)
{
    if (itemID == ITEM_ID_GAIAS_BEACON || itemID == ITEM_ID_UNSTABLE_TESSERACT)
        return 1500;

    if (itemID == ITEM_ID_TECHNO_ORGANIC_ENGINE)
        return 3000;

    return 5000;
}

bool SuckerBlockManager::IsAllowedItemInMachine(int32 itemID, int32 machineID)
{
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
        return false;

    if (pItem->HasFlag(ITEM_FLAG_PERMANENT) || pItem->HasFlag(ITEM_FLAG_AUTOPICKUP) ||
        pItem->HasFlag(ITEM_FLAG_PUBLIC) || pItem->HasFlag(ITEM_FLAG_UNTRADEABLE))
        return false;

    if (pItem->id == ITEM_ID_WATER_BUCKET || pItem->id == ITEM_ID_DIAMOND_HORN || pItem->id == ITEM_ID_DIAMOND_HORNS ||
        pItem->id == ITEM_ID_DIAMOND_DEVIL_HORNS)
        return false;

    if (pItem->type == ITEM_TYPE_SEED)
    {
        if (machineID == ITEM_ID_UNSTABLE_TESSERACT)
            return false;

        ItemInfo* pSeed = GetItemInfoManager()->GetItemByID(SEED_TO_ITEM_ID(pItem->id));
        if (!pSeed)
            return false;

        if (pSeed->HasFlag(ITEM_FLAG_PERMANENT) || pSeed->HasFlag(ITEM_FLAG_AUTOPICKUP) ||
            pSeed->HasFlag(ITEM_FLAG_PUBLIC) || pSeed->HasFlag(ITEM_FLAG_UNTRADEABLE))
            return false;
    }
    else if (machineID == ITEM_ID_GAIAS_BEACON)
        return false;

    if (IsAllowedToBuildOrPlantItem(pItem->id))
        return true;

    switch (pItem->type)
    {
        case ITEM_TYPE_CONSUMABLE:
        case ITEM_TYPE_CLOTHES:
        case ITEM_TYPE_DICE:
        case ITEM_TYPE_COMMAND:
        case ITEM_TYPE_ROOT_CUTTING:
            return true;

        default:
            return false;
    }
}

void SuckerBlockManager::Add(TileInfo* pTile)
{
    if (!pTile || Has(pTile))
        return;

    m_suckerTiles.push_back(pTile);

    if (TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>())
    {
        if (pTileExtra->isPlanting == 1)
        {
            m_pActivePlanter = pTile;

            if (m_pWorld)
            {
                m_pWorld->SendOnPlanterActivatedToAll();
            }
        }
    }
}

void SuckerBlockManager::Remove(TileInfo* pTile)
{
    if (!pTile)
        return;

    auto it = std::find(m_suckerTiles.begin(), m_suckerTiles.end(), pTile);
    if (it != m_suckerTiles.end())
        return;

    m_suckerTiles.erase(it);

    if (TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>())
    {
        if (pTileExtra->isPlanting == 1)
        {
            if (m_pWorld)
            {
                m_pWorld->SendOnPlanterActivatedToAll();
            }
        }
    }
}

bool SuckerBlockManager::Has(TileInfo* pTile)
{
    if (!pTile)
        return false;

    return std::find(m_suckerTiles.begin(), m_suckerTiles.end(), pTile) != m_suckerTiles.end();
}

bool SuckerBlockManager::TogglePlanting(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pTile)
        return false;

    TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>();
    if (!pTileExtra)
        return false;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return false;

    if (!pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
        return false;

    int16 fgID = pTile->GetFG();
    if (fgID == ITEM_ID_GAIAS_BEACON || fgID == ITEM_ID_UNSTABLE_TESSERACT)
    {
        pWorld->SendTileUpdate(pTile);
        return true;
    }

    ItemInfo* pMachineItemInfo = GetItemInfoManager()->GetItemByID(fgID);

    if (pTileExtra->itemID == ITEM_ID_BLANK)
    {
        if (pMachineItemInfo)
        {
            pPlayer->SendOnTalkBubble("You need to select an item first for " + pMachineItemInfo->name + "!", false);
        }

        return false;
    }

    if (!IsAllowedToBuildOrPlantItem(pTileExtra->itemID))
    {
        if (pMachineItemInfo)
        {
            pPlayer->SendOnTalkBubble("This item cannot be planted using " + pMachineItemInfo->name + "!", false);
        }

        return false;
    }

    if (!m_pActivePlanter)
    {
        GiveRemoteToPlayer(pPlayer);
        pTileExtra->isPlanting = 1;
        m_pActivePlanter = pTile;

        pWorld->SendOnPlanterActivatedToAll();
    }
    else if (m_pActivePlanter == pTile)
    {
        if (pTileExtra->isPlanting == 0)
        {
            ReInit();
            return true;
        }

        pTileExtra->isPlanting = 0;
        m_pActivePlanter = nullptr;

        pWorld->SendOnPlanterActivatedToAll();
    }
    else
    {
        TileExtra_Sucker* pPrevExtra = m_pActivePlanter->GetExtra<TileExtra_Sucker>();

        if (pPrevExtra && pPrevExtra->isPlanting == 0)
        {
            ReInit();
            return true;
        }

        pPrevExtra->isPlanting = 0;
        pWorld->SendTileUpdate(m_pActivePlanter);

        m_pActivePlanter = pTile;
        pTileExtra->isPlanting = 1;

        pWorld->SendOnPlanterActivatedToAll();
        GiveRemoteToPlayer(pPlayer);
    }

    pWorld->SendTileUpdate(pTile);
    return true;
}

void SuckerBlockManager::GiveRemoteToPlayer(GamePlayer* pPlayer)
{
    if (!pPlayer)
        return;

    if (pPlayer->GetInventory().GetCountOfItem(ITEM_ID_MAGPLANT_5000_REMOTE) > 0)
        return;

    pPlayer->ModifyInventoryItem(ITEM_ID_MAGPLANT_5000_REMOTE, 1);

    if (ItemInfo* pItem = GetItemInfoManager()->GetItemByID(ITEM_ID_MAGPLANT_5000_REMOTE))
    {
        pPlayer->SendOnTalkBubble("You received a " + pItem->name + "!", false);
    }
}

bool SuckerBlockManager::OnPlayerUsedRemote(GamePlayer* pPlayer)
{
    if (!m_pActivePlanter || !m_pWorld)
        return false;

    if (pPlayer->GetCurrentWorld() != m_pWorld->GetInstanceID())
        return false;

    TileExtra_Sucker* pTileExtra = m_pActivePlanter->GetExtra<TileExtra_Sucker>();
    if (!pTileExtra)
        return false;

    if (pTileExtra->count < 1)
        return false;

    bool needsUpdate = (pTileExtra->count > GetMachineCapacity(m_pActivePlanter->GetFG()) - 50);
    pTileExtra->count -= 1;

    if (needsUpdate || pTileExtra->count == 0)
    {
        m_pWorld->SendTileUpdate(m_pActivePlanter);
    }
    return true;
}

void SuckerBlockManager::Reset()
{
    m_pActivePlanter = nullptr;
    m_suckerTiles.clear();
}

void SuckerBlockManager::ReInit()
{
    Reset();

    if (!m_pWorld)
        return;

    WorldTileManager* pTileMgr = m_pWorld->GetTileManager();
    Vector2Int& vWorldSize = pTileMgr->GetSize();

    std::vector<TileInfo*> tileUpdates;

    for (int32 i = 0; i < (vWorldSize.x * vWorldSize.y); ++i)
    {
        TileInfo* pTile = pTileMgr->GetTile(i);
        if (!pTile)
            continue;

        TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>();
        if (!pTileExtra)
            continue;

        Add(pTile);
        tileUpdates.push_back(pTile);
    }

    m_pWorld->SendTileUpdateMultiple(tileUpdates);
}

int32 SuckerBlockManager::GetItemCountOfPlanter()
{
    if (!m_pActivePlanter)
        return 0;

    TileExtra_Sucker* pTileExtra = m_pActivePlanter->GetExtra<TileExtra_Sucker>();
    if (!pTileExtra)
        return 0;

    return pTileExtra->count;
}
