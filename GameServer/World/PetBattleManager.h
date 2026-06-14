#pragma once

// placeholder for now

#include "Precompiled.h"

class World;
class GamePlayer;
class TileInfo;

enum eBattlePetEvent
{
    PET_EVENT_EQUIP,
    PET_EVENT_BATTLE,
    PET_EVENT_END_GAME,
    PET_EVENT_SWAP,
    PET_EVENT_POWER,
    PET_EVENT_STATUS,
    PET_EVENT_DAMAGE,
    PET_EVENT_HEAL,
    PET_EVENT_DAMAGE_BACK,
    PET_EVENT_HEAL_BACK,
    PET_EVENT_QUEUE,
    PET_EVENT_WEATHER
};

struct PetBattlePetInfo
{
    string name;
    int32 hp = 0;
    int32 maxHp = 0;
    int32 type[3] = { 0 };
    int32 cooldown[3] = { 0 };
};

struct PetBattleClientInfo
{
    GamePlayer* pPlayer = nullptr;
    TileInfo* pTile = nullptr;
    PetBattlePetInfo petInfo;
};

class PetBattle {
public:
    PetBattle(World* pWorld);
    ~PetBattle();

public:
    bool Init(GamePlayer* pPlayer_1, GamePlayer* pPlayer_2, int32 p1_x = -1, int32 p1_y = -1, int32 p2_x = -1, int32 p2_y = -1);

private:
    World* m_pWorld;
    Timer m_lastUpdate;
    PetBattleClientInfo m_players[2];
};

class PetBattleManager {

};