#pragma once

#include "ContextBase.h"
#include "Database/DatabasePool.h"
#include "Utils/GameConfig.h"

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