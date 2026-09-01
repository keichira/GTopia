#pragma once

#include "Precompiled.h"

class World;
class TileInfo;
class TileExtra_Sucker;
class GamePlayer;
struct ItemInfo;

class SuckerBlockManager
{
public:
    SuckerBlockManager(World* pWorld);
    ~SuckerBlockManager();

public:
    static bool IsAllowedToBuildOrPlantItem(int32 itemID);
    static bool IsAllowedItemInMachine(int32 itemID, int32 machineID);
    static int32 GetMachineCapacity(int32 itemID);
    static bool IsRestrictedItem(ItemInfo* pItem);

public:
    int32 GetItemCountOfPlanter() const;
    bool IsCorrupted(TileInfo* pSucker);

    void Add(TileInfo* pTile);
    void Remove(TileInfo* pTile);

    void ChangeSuckerItem(TileInfo* pTile, int32 oldItemID, int32 newItemID);

    bool ToggleRemote(GamePlayer* pPlayer, TileInfo* pTile);
    bool OnPlayerUsedRemote(GamePlayer* pPlayer);
    void GiveRemoteToPlayer(GamePlayer* pPlayer);

    void Reset();
    void ReInit();

    TileInfo* GetSuckerToSuckItemByID(int32 itemID, int32 count);
    TileInfo* GetActivePlanter() { return m_pActivePlanter; }

private:
    void RegisterSuckerTile(TileInfo* pTile, int32 itemID);
    void UnregisterSuckerTile(TileInfo* pTile, int32 itemID);

private:
    World* m_pWorld;
    TileInfo* m_pActivePlanter;
    std::unordered_map<int32, std::vector<TileInfo*>> m_suckerMap;
};