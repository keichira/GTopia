#include "Context.h"
#include "Crash/CrashReport.h"
#include "Server/GameServer.h"
#include "Server/ServerManager.h"
#include "Server/TelnetServer.h"

Context::Context() : m_pDbPool(nullptr), m_pGameConfig(nullptr) {}

Context::~Context() {}

void Context::Init()
{
    ContextBase::Init();

    CrashHandler::Initialize();
    InitializeCrashLogFile("Master", GetID());

    m_pDbPool = new DatabasePool();
    m_pGameConfig = new GameConfig();
}

void Context::Kill()
{
    ContextBase::Kill();

    CloseCrashLogFile();

    GetGameServer()->Kill();
    GetServerManager()->Kill();
    GetTelnetServer()->Kill();

    SAFE_DELETE(m_pDbPool);
    SAFE_DELETE(m_pGameConfig);
}

Context* GetContext()
{
    return Context::GetInstance();
}