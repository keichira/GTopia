#include "Context.h"

Context::Context()
: m_pDbPool(nullptr)
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

Context* GetContext() { return Context::GetInstance(); }
