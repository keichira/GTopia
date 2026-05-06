#pragma once

#include "ContextBase.h"
#include "Database/DatabasePool.h"
#include "Utils/GameConfig.h"

static const uint32 GAME_TICK_MS = 12;
static const uint32 NETWORK_BUDGET_MS = 8;
static const uint32 DB_RESULT_BUDGET_MS = 10;
static const uint32 MAX_CATCHUP_TICKS = 4;
static const uint32 PERF_SAMPLE_INTERVAL_MS = 1000;

class Context : public ContextBase {
public:
    Context();
    ~Context();

public:
    static Context* GetInstance() 
    {
        static Context instance;
        return &instance;
    }

public:
    void Init() override;
    void Kill() override;

public:
    GameConfig* GetGameConfig() { return m_pGameConfig; }
    DatabasePool* GetDatabasePool() { return m_pDbPool; }

private:
    DatabasePool* m_pDbPool;
    GameConfig* m_pGameConfig;
};

Context* GetContext();