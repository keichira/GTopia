#include "Context.h"

Context::Context()
: m_pDbPool(nullptr), m_pGameConfig(nullptr)
{
}

Context::~Context()
{
}

void Context::Init()
{
    m_pDbPool = new DatabasePool();
    m_pGameConfig = new GameConfig();
}

void Context::Kill()
{
    ContextBase::Kill();

    SAFE_DELETE(m_pDbPool);
    SAFE_DELETE(m_pGameConfig);
}

Context* GetContext() { return Context::GetInstance(); }