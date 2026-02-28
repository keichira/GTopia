#pragma once

#include "Precompiled.h"
#include "World/WorldManagerBase.h"
#include "../Player/GamePlayer.h"
#include "World.h"

class WorldManager : public WorldManagerBase {
public:
    WorldManager();
    ~WorldManager();

public:
    static WorldManager* GetInstance()
    {
        static WorldManager instance;
        return &instance;
    }

public:
    void OnHandleDatabase(QueryTaskResult&& result) override;
    void OnHandleTCP(VariantVector&& result) override;

public:
    void PlayerJoinRequest(GamePlayer* pPlayer, const string& worldName);

    World* GetWorldByID(uint32 worldID);
    World* GetWorldByName(const string& worldName);

private:
    std::unordered_map<uint32, World*> m_worlds;
};

WorldManager* GetWorldManager();