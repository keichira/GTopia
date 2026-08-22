#include "SuckerBlockManager.h"
#include "Item/ItemInfoManager.h"
#include "World/WorldManager.h"

SuckerBlockManager::SuckerBlockManager(World* pWorld) : m_pWorld(pWorld), m_pActivePlanter(nullptr) {}

SuckerBlockManager::~SuckerBlockManager() {}

bool SuckerBlockManager::IsRestrictedItem(ItemInfo* pItem)
{
    if (!pItem)
        return true;

    if (pItem->HasFlag(ITEM_FLAG_PERMANENT) || pItem->HasFlag(ITEM_FLAG_AUTOPICKUP) ||
        pItem->HasFlag(ITEM_FLAG_PUBLIC) || pItem->HasFlag(ITEM_FLAG_UNTRADEABLE))
    {
        return true;
    }

    if (pItem->id == ITEM_ID_WATER_BUCKET || pItem->id == ITEM_ID_DIAMOND_HORN || pItem->id == ITEM_ID_DIAMOND_HORNS ||
        pItem->id == ITEM_ID_DIAMOND_DEVIL_HORNS)
    {
        return true;
    }

    return false;
}

bool SuckerBlockManager::IsCorrupted(TileInfo* pSucker)
{
    if (!pSucker)
        return true;

    TileExtra_Sucker* pTileExtra = pSucker->GetExtra<TileExtra_Sucker>();
    if (!pTileExtra)
        return true;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTileExtra->itemID);
    if (!pItem)
    {
        ChangeSuckerItem(pSucker, pTileExtra->itemID, ITEM_ID_BLANK);

        pTileExtra->itemID = 0;
        pTileExtra->count = 0;
        pTileExtra->isSucking = 1;
        pTileExtra->isPlanting = 0;
        return true;
    }

    return false;
}

bool SuckerBlockManager::IsAllowedToBuildOrPlantItem(int32 itemID)
{
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem || IsRestrictedItem(pItem))
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
    if (!pItem || IsRestrictedItem(pItem))
        return false;

    if (pItem->type == ITEM_TYPE_SEED)
    {
        if (machineID == ITEM_ID_UNSTABLE_TESSERACT)
            return false;

        ItemInfo* pSeed = GetItemInfoManager()->GetItemByID(SEED_TO_ITEM_ID(pItem->id));
        if (!pSeed || IsRestrictedItem(pSeed))
            return false;
    }
    else if (machineID == ITEM_ID_GAIAS_BEACON)
    {
        return false;
    }

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
    if (!pTile)
        return;

    TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>();
    if (!pTileExtra)
        return;

    RegisterSuckerTile(pTile, pTileExtra->itemID);

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

    TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>();
    if (pTileExtra)
    {
        UnregisterSuckerTile(pTile, pTileExtra->itemID);
    }

    if (m_pActivePlanter == pTile)
    {
        m_pActivePlanter = nullptr;
    }
}

void SuckerBlockManager::ChangeSuckerItem(TileInfo* pTile, int32 oldItemID, int32 newItemID)
{
    if (!pTile || oldItemID == newItemID)
        return;

    UnregisterSuckerTile(pTile, oldItemID);
    RegisterSuckerTile(pTile, newItemID);
}

bool SuckerBlockManager::TogglePlanting(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile)
        return false;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return false;

    if (!pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
        return false;

    if (pTile->GetFG() == ITEM_ID_GAIAS_BEACON || pTile->GetFG() == ITEM_ID_UNSTABLE_TESSERACT)
        return false;

    TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("The item sucker tile extra is corrupted!", false);
        return false;
    }

    if (pTileExtra->itemID == ITEM_ID_BLANK)
    {
        pPlayer->SendOnTalkBubble("Select an item first!", false);
        if (m_pActivePlanter == pTile)
        {
            m_pActivePlanter = nullptr;
            pTileExtra->isPlanting = 0;
        }
        return false;
    }

    if (!IsAllowedToBuildOrPlantItem(pTileExtra->itemID))
    {
        pPlayer->SendOnTalkBubble("You cannot activate planting mode for this item!", false);
        return false;
    }

    if (m_pActivePlanter == nullptr)
    {
        m_pActivePlanter = pTile;
        pTileExtra->isPlanting = 1;

        pWorld->SendOnPlanterActivatedToAll();
    }
    else if (m_pActivePlanter == pTile)
    {
        m_pActivePlanter = nullptr;
        pTileExtra->isPlanting = 0;

        pWorld->SendOnPlanterActivatedToAll();
    }
    else
    {
        if (m_pActivePlanter)
        {
            TileExtra_Sucker* pOldExtra = m_pActivePlanter->GetExtra<TileExtra_Sucker>();
            if (pOldExtra)
            {
                pOldExtra->isPlanting = 0;
            }
            pWorld->SendTileUpdate(m_pActivePlanter);
        }

        m_pActivePlanter = pTile;
        pTileExtra->isPlanting = 1;

        pWorld->SendOnPlanterActivatedToAll();
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
    if (!pPlayer || !m_pActivePlanter || !m_pWorld)
        return false;

    if (pPlayer->GetCurrentWorld() != m_pWorld->GetInstanceID())
        return false;

    TileExtra_Sucker* pTileExtra = m_pActivePlanter->GetExtra<TileExtra_Sucker>();
    if (!pTileExtra || pTileExtra->count < 1)
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
    m_suckerMap.clear();
}

void SuckerBlockManager::ReInit()
{
    Reset();

    if (!m_pWorld)
        return;

    WorldTileManager* pTileMgr = m_pWorld->GetTileManager();
    if (!pTileMgr)
        return;

    Vector2Int& vWorldSize = pTileMgr->GetSize();
    int32 totalTiles = vWorldSize.x * vWorldSize.y;

    std::vector<TileInfo*> tileUpdates;

    for (int32 i = 0; i < totalTiles; ++i)
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

    if (!tileUpdates.empty())
    {
        m_pWorld->SendTileUpdateMultiple(tileUpdates);
    }
}

TileInfo* SuckerBlockManager::GetSuckerToSuckItemByID(int32 itemID, int32 count)
{
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
        return nullptr;

    if (pItem->id == ITEM_ID_BLANK)
        return nullptr;

    auto it = m_suckerMap.find(pItem->id);
    if (it == m_suckerMap.end())
        return nullptr;

    auto& tiles = it->second;

    for (int32 i = 0; i < tiles.size();)
    {
        TileInfo* pTile = tiles[i];
        if (!pTile)
        {
            tiles[i] = tiles.back();
            tiles.pop_back();
            continue;
        }

        TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>();
        if (!pTileExtra)
        {
            tiles[i] = tiles.back();
            tiles.pop_back();
            continue;
        }

        if (pTileExtra->itemID != pItem->id)
        {
            tiles[i] = tiles.back();
            tiles.pop_back();
            continue;
        }

        if (pTileExtra->isSucking == 0)
        {
            ++i;
            continue;
        }

        if (pTileExtra->count + count <= GetMachineCapacity(pTile->GetFG()))
            return pTile;

        ++i;
    }

    return nullptr;
}

void SuckerBlockManager::RegisterSuckerTile(TileInfo* pTile, int32 itemID)
{
    if (!pTile || itemID == ITEM_ID_BLANK)
        return;

    auto& vec = m_suckerMap[itemID];
    for (TileInfo* pSuckerTile : vec)
    {
        if (pSuckerTile == pTile)
            return;
    }
    vec.push_back(pTile);
}

void SuckerBlockManager::UnregisterSuckerTile(TileInfo* pTile, int32 itemID)
{
    if (!pTile)
        return;

    auto it = m_suckerMap.find(itemID);
    if (it == m_suckerMap.end())
        return;

    auto& suckerTiles = it->second;
    for (int32 i = 0; i < suckerTiles.size(); ++i)
    {
        if (suckerTiles[i] == pTile)
        {
            suckerTiles[i] = suckerTiles.back();
            suckerTiles.pop_back();
            break;
        }
    }

    if (suckerTiles.empty())
    {
        m_suckerMap.erase(it);
    }
}

int32 SuckerBlockManager::GetItemCountOfPlanter() const
{
    if (!m_pActivePlanter)
        return 0;

    TileExtra_Sucker* pTileExtra = m_pActivePlanter->GetExtra<TileExtra_Sucker>();
    if (!pTileExtra)
        return 0;

    return pTileExtra->count;
}