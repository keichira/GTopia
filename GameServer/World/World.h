#pragma once

#include "Precompiled.h"
#include "World/WorldInfo.h"

class GamePlayer;

class World : public WorldInfo {
public:
    World();
    ~World();

public:
    void SetID(uint32 id) { m_worldID = id; }
    uint32 GetID() const { return m_worldID; }

    bool PlayerJoinWorld(GamePlayer* pPlayer);

private:
    uint32 m_worldID;
    std::vector<GamePlayer*> m_players;
};