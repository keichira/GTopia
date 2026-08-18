#include "Context.h"
#include "Crash/CrashReport.h"
#include "Server/GameServer.h"
#include "Server/MasterBroadway.h"

Context::Context() : m_pDbPool(nullptr) {}

Context::~Context() {}

void Context::Init()
{
    ContextBase::Init();

    CrashHandler::Initialize();
    InitializeCrashLogFile("GameServer", GetID());

    m_pDbPool = new DatabasePool();
    m_pGameConfig = new GameConfig();
}

void Context::Kill()
{
    ContextBase::Kill();
    CloseCrashLogFile();

    GetMasterBroadway()->Kill();
    GetGameServer()->Kill();

    SAFE_DELETE(m_pDbPool);
    SAFE_DELETE(m_pGameConfig);
}

Context* GetContext()
{
    return Context::GetInstance();
}
