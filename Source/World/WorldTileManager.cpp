#include "WorldTileManager.h"
#include "../Math/Math.h"
#include "../Item/ItemInfoManager.h"
#include "../Math/Random.h"
#include "WorldInfo.h"
#include "../Utils/StringUtils.h"
#include "../Utils/GrowUtils.h"

WorldTileManager::WorldTileManager()
: m_size(WORLD_DEFAULT_WIDTH, WORLD_DEFAULT_HEIGHT)
{
    m_keyTiles.resize(KEY_TILE_SIZE, nullptr);
    m_tempTiles.resize(m_size.x * m_size.y);
}

WorldTileManager::~WorldTileManager()
{
    Clear();
}

bool WorldTileManager::Serialize(MemoryBuffer& memBuffer, bool write, bool database, WorldInfo* pWorld, float gameVersion)
{
    memBuffer.ReadWrite(m_size, write);

    uint32 totalTiles = m_tiles.size();
    memBuffer.ReadWrite(totalTiles, write);

    if(totalTiles != (m_size.x * m_size.y) || totalTiles > (255*255)) {
        return false;
    }

    if(!write) {
        m_tiles.resize(totalTiles);
        m_tempTiles.resize(totalTiles);
    }

    if(write && !database && gameVersion >= 3.31f)
    {
        uint32 val1 = 0;
        uint8 val2 = 0;
        memBuffer.ReadWrite(val1, write);
        memBuffer.ReadWrite(val2, write);
    }

    if(write) {
        if(database) {
            memBuffer.WriteRaw(m_tempTiles.data(), sizeof(TempTileData) * m_tempTiles.size());
            
            uint32 extraCount = 0;
            uint32 checkpoint = memBuffer.GetOffset();
            memBuffer.Write(extraCount);

            for(uint16 i = 0; i < m_tiles.size(); ++i) {
                TileInfo* pTile = &m_tiles[i];

                if(!pTile->HasFlag(TILE_FLAG_HAS_EXTRA_DATA) && !pTile->HasFlag(TILE_FLAG_HAS_PARENT)) {
                    continue;
                }

                extraCount++;
                memBuffer.Write(i);
                pTile->Serialize(memBuffer, true, true, pWorld->GetWorldVersion());
            }

            uint32 end = memBuffer.GetOffset();

            memBuffer.Seek(checkpoint);
            memBuffer.Write(extraCount);
            
            memBuffer.Seek(end);
        }
        else {
            uint32 batchStartIdx = 0;

            for(uint32 i = 0; i < m_tiles.size(); ++i) {
                TileInfo* pTile = &m_tiles[i];

                if(!pTile->HasFlag(TILE_FLAG_HAS_EXTRA_DATA) && !pTile->HasFlag(TILE_FLAG_HAS_PARENT)) {
                    continue;
                }
    
                uint32 batchCount = i - batchStartIdx;

                if(batchCount > 0) {
                    memBuffer.WriteRaw(m_tempTiles.data() + batchStartIdx, sizeof(TempTileData) * batchCount);
                }

                pTile->Serialize(memBuffer, true, false, pWorld->GetWorldVersion());
                batchStartIdx = i + 1;
            }
    
            uint32 remaining = m_tempTiles.size() - batchStartIdx;
            if(remaining > 0) {
                memBuffer.WriteRaw(m_tempTiles.data() + batchStartIdx, sizeof(TempTileData) * remaining);
            }
        }
    }
    else {
        if(database) {
            memBuffer.ReadRaw(m_tempTiles.data(), m_tempTiles.size() * sizeof(TempTileData));

            for(uint32 i = 0; i < m_tiles.size(); ++i) {
                TileInfo* pTile = &m_tiles[i];

                pTile->BindTileData(&m_tempTiles[i]);
                ModifyKeyTile(pTile, false);
                pTile->SetPos(i % m_size.x, i / m_size.x);
                pTile->SetMapIndex(i);
            }

            uint32 extraCount = 0;
            memBuffer.Read(extraCount);

            for(uint32 i = 0; i < extraCount; ++i) {
                uint16 tileIdx = 55555;
                memBuffer.Read(tileIdx);

                if(tileIdx >= m_tiles.size()) {
                    LOGGER_LOG_ERROR("Tile extra corrupted while reading %s idx:%d tilesize:%d", pWorld->GetWorldVersion(), tileIdx, m_tiles.size());
                    return false;
                }

                TileInfo* pTile = &m_tiles[tileIdx];
                pTile->Serialize(memBuffer, false, true, pWorld->GetWorldVersion());
            }
        }
        else {
            /*for(uint32 i = 0; i < m_tiles.size(); ++i) { // unused but yeah...
                TileInfo* pTile = &m_tiles[i];

                pTile->Serialize(memBuffer, false, false, pWorld);
                ModifyKeyTile(&m_tiles[i], false);
        
                pTile->SetPos(i % m_size.x, i / m_size.x);
            }*/
        }
    }

    return true;
}

uint32 WorldTileManager::GetMemEstimate(bool database, WorldInfo* pWorld, float gameVersion)
{
    MemoryBuffer memSize;

    Serialize(memSize, true, database, pWorld, gameVersion);
    return memSize.GetOffset();
}

void WorldTileManager::Clear(bool reInit)
{
    m_tiles.clear();
    m_keyTiles.clear();
    m_onFireTiles.clear();
    m_tempTiles.clear();

    if(reInit) {
        m_tiles.resize(m_size.x * m_size.y);
        m_tempTiles.resize(m_size.x * m_size.y);
        m_keyTiles.resize(KEY_TILE_SIZE, nullptr);
        m_tempTiles.resize(m_size.x * m_size.y);

        for(uint32 i = 0; i < m_tiles.size(); ++i) { 
            m_tiles[i].BindTileData(&m_tempTiles[i]);
            m_tiles[i].SetPos(i % m_size.x, i / m_size.x); 
            m_tiles[i].SetMapIndex(i);
        }
    }
}

TileInfo* WorldTileManager::GetTile(const Vector2Int& pos)
{
    return GetTile(pos.x, pos.y);
}

TileInfo *WorldTileManager::GetTile(int32 x, int32 y)
{
    if(x < 0 || y < 0 || x >= m_size.x || y >= m_size.y) {
        return nullptr;
    }

    return &m_tiles[y * m_size.x + x];
}

TileInfo* WorldTileManager::GetKeyTile(eKeyTile keyTile)
{
    return m_keyTiles[keyTile];
}

TileInfo* WorldTileManager::GetTile(int32 index)
{
    if(index < 0 || index > (m_size.x * m_size.y)) {
        return nullptr;
    }

    if(index >= m_tiles.size())
        return nullptr;

    return &m_tiles[index];
}

TileInfo* WorldTileManager::GetTileByWorldPos(float x, float y)
{
    if(x <= 0 || y <= 0)
        return nullptr;

    return GetTile((int32)(x / 32.0f), (int32)(y / 32.0f));
}

TileInfo* WorldTileManager::GetTileByWorldPos(const Vector2Float& pos)
{
    return GetTileByWorldPos(pos.x, pos.y);
}

int32 WorldTileManager::GetTileIndex(TileInfo* pTile)
{
    if(!pTile)
        return -1;

    Vector2Int vTilePos = pTile->GetPos();
    if(vTilePos.x < 0 || vTilePos.y < 0 || vTilePos.x >= m_size.x || vTilePos.y >= m_size.y) {
        return -1;
    }

    return vTilePos.y * m_size.x + vTilePos.x;
}

bool WorldTileManager::CanPlantTreeHere(TileInfo *pTile)
{
    if(!pTile)
        return false;

    Vector2Int tilePos = pTile->GetPos();
    TileInfo* pBottom = GetTile(tilePos.x, tilePos.y + 1);
    if(!pBottom)
        return false;

    return pBottom->IsCollidable();
}

void WorldTileManager::ModifyKeyTile(TileInfo* pTile, bool remove)
{
    TileInfo* pKeyTile = nullptr;

    uint32 fgItem = pTile->GetFG();

    if(IsMainDoor(fgItem)) {
        m_keyTiles[KEY_TILE_MAIN_DOOR] = remove ? nullptr : pTile;
    }
    else if(fgItem == ITEM_ID_GUARDIAN_PINEAPPLE) {
        m_keyTiles[KEY_TILE_GUARD_PINEAPPLE] = remove ? nullptr : pTile;
    }
    else if(IsWorldLock(fgItem)) {
        m_keyTiles[KEY_TILE_WORLD_LOCK] = remove ? nullptr : pTile;
    }
    else if(fgItem == ITEM_ID_PUNCH_JAMMER) {
        m_keyTiles[KEY_TILE_PUNCH_JAMMER] = remove ? nullptr : pTile;
    }
    else if(fgItem == ITEM_ID_ZOMBIE_JAMMER) {
        m_keyTiles[KEY_TILE_ZOMBIE_JAMMER] = remove ? nullptr : pTile;
    }
    else if(fgItem == ITEM_ID_SIGNAL_JAMMER) {
        m_keyTiles[KEY_TILE_SIGNAL_JAMMER] = remove ? nullptr : pTile;
    }
    else if(fgItem == ITEM_ID_ANTIGRAVITY_GENERATOR) {
        m_keyTiles[KEY_TILE_ANTIGRAVITY] = remove ? nullptr : pTile;
    }
    else if(fgItem == ITEM_ID_GHOST_CHARM) {
        m_keyTiles[KEY_TILE_GHOST_CHARM] = remove ? nullptr : pTile;
    }
    else if(fgItem == ITEM_ID_XENONITE_CRYSTAL) {
        m_keyTiles[KEY_TILE_XENONITE] = remove ? nullptr : pTile;
    }
    else if(fgItem == ITEM_ID_CONTAINMENT_FIELD_POWER_NODE)
    {
        if(!remove)
        {
            if(std::find(m_powerNodes.begin(), m_powerNodes.end(), pTile) == m_powerNodes.end())
            {
                m_powerNodes.push_back(pTile);
            }

            uint32 tileIdx = pTile->GetMapIndex();

            for(auto& pNode : m_powerNodes)
            {
                if(pNode == pTile)
                    continue;

                TileExtra_FieldNode* pTileExtra = pNode->GetExtra<TileExtra_FieldNode>();
                if(!pTileExtra)
                    continue;

                bool found = false;

                for(int32 node : pTileExtra->nodes)
                {
                    if(node == tileIdx)
                    {
                        found = true;
                        break;
                    }
                }

                if(!found)
                {
                    pTileExtra->nodes.push_back(tileIdx);
                }
            }

            if(pTile->HasFlag(TILE_FLAG_IS_ON))
            {
                RebuildPowerNodeGroups();
            }
        }
        else
        {
            CheckPowerNodeToKill(pTile);
        }
    }
}

void WorldTileManager::GenerateDefaultMap()
{
    Clear(true);

    RectInt layer(0, 0, m_size.x, m_size.y);
    FillRectWithThickness(6, layer, ITEM_ID_BEDROCK, ITEM_ID_CAVE_BACKGROUND, 100);  
    FillRectWithThickness(4, layer,
            { { ITEM_ID_DIRT, 60 }, { ITEM_ID_LAVA, 40 }, { ITEM_ID_ROCK, 2 } },
            { { ITEM_ID_CAVE_BACKGROUND, 100 } });
    FillRectWithThickness(25, layer,
            { { ITEM_ID_DIRT, 94 }, { ITEM_ID_ROCK, 6 } },
            { { ITEM_ID_CAVE_BACKGROUND, 100 } }); 
    FillRectWithThickness(1, layer, ITEM_ID_DIRT, ITEM_ID_CAVE_BACKGROUND, 100); 

    int32 doorPosX = RandomRangeInt(10, layer.Width() - 10);
    TileInfo* pDoorTile = GetTile(doorPosX, layer.top - 1);

    pDoorTile->SetFG(ITEM_ID_MAIN_DOOR, this);
    if(TileExtra_Door* pTileExtra = pDoorTile->GetExtra<TileExtra_Door>()) {
        pTileExtra->name = "EXIT";
    }

    TileInfo* pBedrockTile = GetTile(doorPosX, layer.top);
    pBedrockTile->SetFG(ITEM_ID_BEDROCK, this);
}

void WorldTileManager::GenerateClearMap()
{
    Clear(true);

    RectInt layer(0, 0, m_size.x, m_size.y);
    FillRectWithThickness(6, layer, ITEM_ID_BEDROCK, ITEM_ID_CAVE_BACKGROUND, 100);  

    bool mainDoorAtRight = RandomRangeInt(0, 1) == 1;
    
    TileInfo* pDoorTile = GetTile( mainDoorAtRight ? m_size.x : 0, layer.top - 1 );
    pDoorTile->SetFG(ITEM_ID_MAIN_DOOR, this);
    
    if(TileExtra_Door* pTileExtra = pDoorTile->GetExtra<TileExtra_Door>()) {
        pTileExtra->name = "EXIT";
    }
}

void WorldTileManager::GenerateBeachMap()
{
    Clear(true);

    bool startFromLeft = RandomRangeInt(0, 2) == 1;
}

void WorldTileManager::FillRectWith(const RectInt& rect, uint16 fgItem, uint16 bgItem, float chance)
{
    if(chance > 100.0f) {
        chance = 100.0f;
    }
    if(chance <= 0.0f) {
        return;
    }

    int32 xStart = Max(0, Min(rect.left, rect.right));
    int32 xEnd = Min(m_size.x, Max(rect.left, rect.right));
    int32 yStart = Max(0, Min(rect.top, rect.bottom));
    int32 yEnd = Min(m_size.y, Max(rect.top, rect.bottom));

    ItemInfoManager* pItemManager = GetItemInfoManager();
    ItemInfo* pItemFg = pItemManager->GetItemByID(fgItem);
    ItemInfo* pItemBg = pItemManager->GetItemByID(bgItem);

    float probablity = chance * 0.01f;

    for(auto y = yStart; y < yEnd; ++y) {
        TileInfo* pTile = &m_tiles[y * m_size.x + xStart];

        for(auto x = xStart; x < xEnd; ++x) {
            if(RandomNextFloat() <= probablity) {
                if(pItemFg) {
                    pTile->SetFG(pItemFg->id, this);
                }
                
                if(pItemBg) {
                    pTile->SetBG(pItemBg->id);
                }
            }

            pTile++;
        }
    }
}

bool WorldTileManager::FillRectWith(const RectInt& rect, const TileMapFillVector& fgItems, const TileMapFillVector& bgItems)
{
    if((fgItems.empty() && bgItems.empty())) {
        return false;
    }

    int32 xStart = Max(0, Min(rect.left, rect.right));
    int32 xEnd = Min(m_size.x, Max(rect.left, rect.right));
    int32 yStart = Max(0, Min(rect.top, rect.bottom));
    int32 yEnd = Min(m_size.x, Max(rect.top, rect.bottom));

    float totalFgChance = 0.0f;
    for(const auto& item : fgItems) {
        totalFgChance += Max(0.0f, Min(100.0f, item.chance));
    }

    float totalBgChance = 0.0f;
    for(const auto& item : bgItems) {
        totalBgChance += Max(0.0f, Min(100.0f, item.chance));
    }

    ItemInfoManager* pItemManager = GetItemInfoManager();

    auto PickItem = [&](const TileMapFillVector& items, float totalChance) -> ItemInfo*
    {
        if(totalChance <= 0.f)
            return nullptr;

        float roll = RandomNextFloat() * totalChance;
        float chance = 0.f;

        for(const auto& entry : items)
        {
            chance += Max(0.0f, Min(100.0f, entry.chance));
            if(roll <= chance)
                return pItemManager->GetItemByID(entry.itemID);
        }

        return nullptr;
    };

    for(auto y = yStart; y < yEnd; ++y) {
        TileInfo* pTile = &m_tiles[y * m_size.x + xStart];

        for(auto x = xStart; x < xEnd; ++x) {

            if(ItemInfo* pBgItem = PickItem(bgItems, totalBgChance)) {
                pTile->SetBG(pBgItem->id);
            }

            if(ItemInfo* pFgItem = PickItem(fgItems, totalFgChance)) {
                pTile->SetFG(pFgItem->id, this);
            }

            pTile++;
        }
    }

    return true;
}

bool WorldTileManager::IsSameTile(TileInfo* pTile, int32 x, int32 y, bool forBackground)
{
    TileInfo* pTarget = GetTile(x, y);
    if(!pTarget) {
        return false;
    }

    if(forBackground) {
        return pTile->GetBG() == pTarget->GetBG();
    }

    return pTile->GetFG() == pTarget->GetFG();
}

TileInfo* WorldTileManager::GetTileByTypeFromRect(const RectFloat& rect, int32 itemType)
{
    int32 xStart = Max(0, Min(rect.left / 32, rect.right / 32));
    int32 xEnd = Min(m_size.x, Max(rect.left / 32, rect.right / 32));
    int32 yStart = Max(0, Min(rect.top / 32, rect.bottom / 32));
    int32 yEnd = Min(m_size.y, Max(rect.top / 32, rect.bottom / 32));

    // todo add type to TileInfo instead of this
    ItemInfoManager* pItemMgr = GetItemInfoManager();

    for(int y = yStart; y < yEnd; ++y)
    {
        for(int x = xStart; x < xEnd; ++x)
        {
            TileInfo* pTile = &m_tiles[y * m_size.x + x];
    
            ItemInfo* pItem = pItemMgr->GetItemByID(pTile->GetDisplayedItem());
            if(!pItem)
                continue;
    
            if(pItem->type == itemType)
                return pTile;
        }
    }

    return nullptr;
}

std::vector<TileInfo*> WorldTileManager::RemoveTileParentsLockedBy(TileInfo* pLockTile)
{
    std::vector<TileInfo*> unlockedTiles;

    if(!pLockTile) {
        return unlockedTiles;
    }

    Vector2Int vLockPos = pLockTile->GetPos();
    uint32 index = vLockPos.x + m_size.x * vLockPos.y;

    for(auto& tile : m_tiles) {
        if(tile.GetParent() == index) {
            tile.SetParent(0);
            tile.RemoveFlag(TILE_FLAG_HAS_PARENT);

            unlockedTiles.push_back(&tile);
        }
    }

    return unlockedTiles;
}

bool WorldTileManager::AbleToLockThisTile(TileInfo* pLockTile, TileInfo* pTargetTile, bool ignoreEmpty)
{
    if(!pLockTile || !pTargetTile || pLockTile == pTargetTile) {
        return false;
    }

    if(pTargetTile->HasFlag(TILE_FLAG_HAS_PARENT) || pTargetTile->GetParent() != 0) {
        return false;
    }

    if(ignoreEmpty && pTargetTile->GetDisplayedItem() == ITEM_ID_BLANK) {
        return false;
    }   

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTargetTile->GetDisplayedItem());
    if(!pItem) {
        return false;
    }

    if(pItem->type == ITEM_TYPE_LOCK || pItem->type == ITEM_TYPE_DOOR || pItem->type == ITEM_TYPE_BEDROCK) {
        return false;
    }

    int32 neighbors[4][2] = 
    {
        {1, 0}, {0, 1}, {-1, 0}, {0, -1}
    };
    Vector2Int vTargetPos = pTargetTile->GetPos();
    Vector2Int vLockPos = pLockTile->GetPos();

    uint32 parentIndex = vLockPos.x + m_size.x * vLockPos.y;

    for(int8 i = 0; i < 4; ++i) {
        TileInfo* pNeighbor = GetTile(vTargetPos.x + neighbors[i][0], vTargetPos.y + neighbors[i][1]);
        if(!pNeighbor) {
            continue;
        }

        if(
            pNeighbor->GetParent() == parentIndex || pNeighbor == pLockTile ||
            (vTargetPos.x + neighbors[i][0] == vLockPos.x + neighbors[i][0] && vTargetPos.y + neighbors[i][0] == vLockPos.y + neighbors[i][0])
        ) {
            return true;
        }
    }

    return false;
}

bool WorldTileManager::ApplyLockTiles(TileInfo* pLockTile, int32 tileSizeToLock, bool ignoreEmpty, std::vector<TileInfo*>& outTiles)
{
    if(!pLockTile || tileSizeToLock > (m_size.x * m_size.y) || tileSizeToLock == 0) {
        return false;
    }

    RemoveTileParentsLockedBy(pLockTile);

    Vector2Int vStartPos = pLockTile->GetPos();
    std::vector<TileInfo*> lockedTiles;

    int32 radius = 1;
    int32 totalLocked = 0;
    uint32 maxRadius = Max(m_size.x, m_size.y);

    while(totalLocked < tileSizeToLock && radius <= maxRadius) { // :/ is it really ok? is that O(n^3)???
        int32 minDistance = 111111111;
        TileInfo* pClosestTile = nullptr;
        
        for(int32 x = vStartPos.x - radius; x <= vStartPos.x + radius; ++x) {
            for(int32 y = vStartPos.y - radius; y <= vStartPos.y + radius; ++y) {
                TileInfo* pCurrTile = GetTile(x, y);
                if(!pCurrTile || pCurrTile == pLockTile) {
                    continue;
                }

                if(!AbleToLockThisTile(pLockTile, pCurrTile, ignoreEmpty)) {
                    continue;
                }

                int32 distance = Abs(x - vStartPos.x) + Abs(y - vStartPos.y);
                if(!pClosestTile || distance < minDistance) {
                    pClosestTile = pCurrTile;
                    minDistance = distance;
                }
            }
        }

        if(!pClosestTile) {
            radius++;
            continue;
        }

        pClosestTile->SetParent(vStartPos.x + m_size.x * vStartPos.y);
        pClosestTile->SetFlag(TILE_FLAG_HAS_PARENT);
        lockedTiles.push_back(pClosestTile);
        totalLocked++;
    }

    outTiles = std::move(lockedTiles);
    return true;
}

void WorldTileManager::AgeTiles(uint32 ageMS)
{
    for(auto& tile : m_tiles)
    {
        tile.AgeTile(ageMS);
    }
}

bool WorldTileManager::RandomizeOuijaBoardTile(TileInfo* pTile)
{
    if(!pTile)
        return false;

    TileExtra_OuijaBoard* pTileExtra = pTile->GetExtra<TileExtra_OuijaBoard>();
    if(!pTileExtra)
        return false;

    bool isDarkSpiritBoard = pTile->GetFG() == ITEM_ID_DARK_SPIRIT_BOARD;

    static std::vector<string> ouijaCommand =
    {
        "/furious",
        "/rolleyes",
        "/idk",
        "/omg",
        "/wave",
        "dance",
        "/love",
        "/sleep",
        "/fp",
        "/fold",
        "/yes",
        "/no"
    };
    
    if(isDarkSpiritBoard)
    {
        pTileExtra->playerCount = 5;
    }
    else
    {
        pTileExtra->playerCount = 2;
    }
    
    auto items = GetOuijaBoardCloth(isDarkSpiritBoard);
    if(items.empty())
        return false;

    pTileExtra->command = ouijaCommand[RandomRangeInt(0, ouijaCommand.size() - 1)];
    pTileExtra->ouijaType = ToString(isDarkSpiritBoard);

    uint32 totalAssigned = 0;

    for(auto& elem : items)
    {
        int32 remainingPlayers = pTileExtra->playerCount - totalAssigned;
        if(remainingPlayers < 1)
            break;

        pTileExtra->items.push_back(elem.bodyPart);
        pTileExtra->items.push_back(elem.itemID);
        
        int32 minCount = (isDarkSpiritBoard) ? 1 : (pTileExtra->playerCount / 2 + 1);
        minCount = Max(1, Min(minCount, pTileExtra->playerCount));

        int32 giveCount = RandomRangeInt(minCount, remainingPlayers);
        if(giveCount > remainingPlayers)
        {
            giveCount = remainingPlayers;
        }

        pTileExtra->items.push_back(giveCount);
        totalAssigned += giveCount;
    }

    return true;
}

bool WorldTileManager::IsPowerNodeActiveInAGroup(TileInfo* pTile)
{
    return m_nodeToGroup.find(pTile) != m_nodeToGroup.end();
}

void WorldTileManager::CheckPowerNodeToKill(TileInfo *pTile)
{
    if(!pTile)
        return;

    auto it = m_nodeToGroup.find(pTile);
    if(it == m_nodeToGroup.end())
        return;

    uint32 groupIdx = it->second;

    if(groupIdx >= m_powerNodeGroups.size())
        return;

    int32 targetId = pTile->GetMapIndex();

    for(auto& pNode : m_powerNodeGroups[groupIdx])
    {
        TileExtra_FieldNode* pTileExtra = pNode->GetExtra<TileExtra_FieldNode>();
        if(pTileExtra)
        {
            auto it = std::find(pTileExtra->nodes.begin(), pTileExtra->nodes.end(), targetId);        
            if(it != pTileExtra->nodes.end())
            {
                pTileExtra->nodes.erase(it);
            }
        }

        m_nodeToGroup.erase(pNode);
    }

    m_powerNodeGroups.erase(m_powerNodeGroups.begin() + groupIdx);
    m_nodeToGroup.clear();

    for(uint32 i = 0; i < m_powerNodeGroups.size(); ++i)
    {
        for(auto& pNode : m_powerNodeGroups[i])
        {
            m_nodeToGroup[pNode] = i;
        }
    }
}

void WorldTileManager::RebuildPowerNodeGroups()
{
    m_powerNodeGroups.clear();
    m_nodeToGroup.clear();
    if(m_powerNodes.size() < 4)
        return;

    std::vector<std::vector<TileInfo*>> candidates;
    candidates.reserve(m_powerNodes.size());

    std::vector<std::pair<TileInfo*, float>> nearest;
    nearest.reserve(m_powerNodes.size());

    std::vector<TileInfo*> currentGroup;
    currentGroup.reserve(4);

    for(auto& pBaseTile : m_powerNodes)
    {
        if(!pBaseTile || !pBaseTile->HasFlag(TILE_FLAG_IS_ON) || IsPowerNodeActiveInAGroup(pBaseTile))
            continue;

        nearest.clear();
        for(auto& pOther : m_powerNodes)
        {
            if(pOther == pBaseTile || !pOther || !pOther->HasFlag(TILE_FLAG_IS_ON) || IsPowerNodeActiveInAGroup(pOther))
                continue;

            float distance = DistanceBetweenPoints(pBaseTile->GetWorldPos(), pOther->GetWorldPos());
            if(distance <= 0.0f || distance > 32.0f * 25) 
                continue;

            nearest.push_back({ pOther, distance });
        }

        if(nearest.size() < 3)
            continue;

        std::partial_sort(nearest.begin(), nearest.begin() + 3, nearest.end(), [](const auto& a, const auto& b) {
            return a.second < b.second;
        });

        currentGroup.clear();
        currentGroup.push_back(pBaseTile);
        for(uint32 i = 0; i < 3; ++i)
        {
            currentGroup.push_back(nearest[i].first);
        }

        std::sort(currentGroup.begin(), currentGroup.end());
        candidates.push_back(currentGroup);
    }
    
    if(candidates.empty())
        return;

    std::sort(candidates.begin(), candidates.end());

    for(uint32 i = 0; i < candidates.size();)
    {
        uint32 j = i + 1;
        while(j < candidates.size() && candidates[i] == candidates[j])
        {
            j++;
        }

        uint32 groupCount = j - i;

        if(groupCount == 4)
        {
            uint32 expireTime = Time::GetSystemTime() + 45000;
            uint32 groupIdx = m_powerNodeGroups.size();
            
            for(auto& pNode : candidates[i])
            {
                m_nodeToGroup[pNode] = groupIdx;
                TileExtra_FieldNode* pTileExtra = pNode->GetExtra<TileExtra_FieldNode>();
                if(pTileExtra)
                {
                    pTileExtra->expireTime = expireTime;
                }
            }
            m_powerNodeGroups.push_back(candidates[i]);
        }

        i = j;
    }
}

TileInfo* WorldTileManager::GetClosestPowerNodeFromWorldPos(const Vector2Float& pos)
{
    TileInfo* pClosestTile = nullptr;
    float minDistance = 9999999.0f;

    for(auto& pNode : m_powerNodes)
    {
        float distance = DistanceBetweenPoints(pNode->GetWorldPos(), pos);

        if(distance < minDistance)
        {
            pClosestTile = pNode;
            minDistance = distance;
        }
    }

    return pClosestTile;
}

bool WorldTileManager::CheckIfPointInsidePowerNodeGroups(const Vector2Float& pos)
{
    if(m_powerNodeGroups.empty())
        return false;

    for(auto& group : m_powerNodeGroups)
    {
        if(group.size() != 4)
            continue;

        Vector2Float vNodePos_1 = group[0]->GetWorldPos();
        Vector2Float vNodePos_2 = group[1]->GetWorldPos();
        Vector2Float vNodePos_3 = group[2]->GetWorldPos();
        Vector2Float vNodePos_4 = group[3]->GetWorldPos();

        if(IsPointInPolygon({vNodePos_1, vNodePos_2, vNodePos_3, vNodePos_4}, pos))
            return true;
    }

    return false;
}

void WorldTileManager::FillRectWithThickness(uint16 thickness, RectInt& rect, uint16 fgItem, uint16 bgItem, float chance)
{
    rect.top = rect.bottom - thickness;
    FillRectWith(rect, fgItem, bgItem, chance);
    rect.bottom = rect.top;
}

void WorldTileManager::FillRectWithThickness(uint16 thickness, RectInt& rect, const TileMapFillVector& fgItems, const TileMapFillVector& bgItems)
{
    rect.top = rect.bottom - thickness;
    FillRectWith(rect, fgItems, bgItems);
    rect.bottom = rect.top;
}
