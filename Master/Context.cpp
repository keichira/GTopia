#include "Context.h"
#include "Crash/CrashReport.h"
#include "Database/DatabaseBootstrapper.h"
#include "Holiday/HolidayManager.h"
#include "Server/AdminServer.h"
#include "Server/GameServer.h"
#include "Server/ServerManager.h"
#include "Server/StatsManager.h"

void ProcessDatabaseResults(uint64 maxTimeMS)
{
    DatabasePool* pDatabasePool = GetContext()->GetDatabasePool();
    if (!pDatabasePool)
        return;

    Timer startTime;
    QueryTaskResult taskRes;

    while (pDatabasePool->GetResult(taskRes))
    {
        if (taskRes.callback)
        {
            taskRes.callback(std::move(taskRes));
            taskRes.Destroy();
        }

        if (startTime.GetElapsedTime() >= maxTimeMS)
            break;
    }
}

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
    GetAdminServer()->Kill();

    SAFE_DELETE(m_pDbPool);
    SAFE_DELETE(m_pGameConfig);
}

bool Context::LoadDatabaseModules()
{
    DatabaseBootstrapper bootstrapper;
    bootstrapper
        .AddTask(
            "StatsManager", [](DatabasePool* db) { GetStatsManager()->LoadFromDatabase(db); },
            []() { return GetStatsManager()->IsLoaded(); })

        .AddTask(
            "HolidayManager", [](DatabasePool* db) { GetHolidayManager()->LoadFromDatabase(db); },
            []() { return GetHolidayManager()->IsLoaded(); });

    if (!bootstrapper.Execute(GetContext()->GetDatabasePool(), ProcessDatabaseResults, GetShutdownFlag(), 15000))
    {
        LOGGER_LOG_ERROR_FLUSH("Critical DB boot failed or server shutdown requested. Terminating Master...");
        GetContext()->Kill();
        return false;
    }

    return true;
}

Context* GetContext()
{
    return Context::GetInstance();
}