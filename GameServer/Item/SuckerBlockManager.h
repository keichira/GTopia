#pragma once

#include "World/TileInfo.h"

class World;
class GamePlayer;

class SuckerBlockManager
{
public:
    SuckerBlockManager(World* pWorld);

public:
    static bool IsCorrupted(TileExtra_Sucker* pSucker);
    static bool IsAllowedToBuildOrPlantItem(int32 itemID);
    static int32 GetMachineCapacity(int32 itemID);
    static bool IsAllowedItemInMachine(int32 itemID, int32 machineID);

public:
    void Add(TileInfo* pTile);
    void Remove(TileInfo* pTile);
    bool Has(TileInfo* pTile);

    bool TogglePlanting(GamePlayer* pPlayer, TileInfo* pTile);
    void GiveRemoteToPlayer(GamePlayer* pPlayer);
    bool OnPlayerUsedRemote(GamePlayer* pPlayer);

    void Reset();
    void ReInit();

    int32 GetItemCountOfPlanter();

    TileInfo* GetActivePlanter() { return m_pActivePlanter; };

private:
    TileInfo* m_pActivePlanter;
    std::vector<TileInfo*> m_suckerTiles;
    World* m_pWorld;
};