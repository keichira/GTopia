#include "Command/CommandManager.h"
#include "Context.h"
#include "Holiday/HolidayManager.h"
#include "IO/Log.h"
#include "Item/ItemInfoManager.h"
#include "Math/Math.h"
#include "Math/Random.h"
#include "Player/GamePlayer.h"
#include "Player/PlayModManager.h"
#include "Player/RoleManager.h"
#include "Server/AdminServer.h"
#include "Server/GameServer.h"
#include "Server/ServerManager.h"
#include "Server/StatsManager.h"
#include "Utils/Timer.h"
#include "World/WorldManager.h"

// clang-format off
#ifdef _WIN32
#include <windows.h>
#include <timeapi.h>
#endif
// clang-format on

bool firstCallShutdown = false;

void ForceSaveEverything()
{
    firstCallShutdown = true;

    GetStatsManager()->FlushToDatabase(GetContext()->GetDatabasePool(), true);

    if (!GetContext()->GetDatabasePool()->GetWorker(0)->IsConnected())
    {
        GetContext()->Stop();
    }
    else
    {
        QueryRequest req;
        req.callback = [](QueryTaskResult&&) { GetContext()->Stop(); };
        DatabaseExec(GetContext()->GetDatabasePool(), "", req, QUERY_FLAG_NONE);
    }
}

void DatabaseThreadFunc()
{
    Timer logTimer;
    Context* pContext = GetContext();
    DatabaseWorker* pWorker = pContext->GetDatabasePool()->GetWorker(0);

    while (pContext->IsRunning())
    {
        if (pWorker)
        {
            pWorker->Update();
        }

        if (logTimer.GetElapsedTime() >= 5000)
        {
            GetLog()->Write();
            logTimer.Reset();
        }
        SleepMS(1);
    }
}

void EventThreadFunc()
{
    GameServer* pGameServer = GetGameServer();
    ServerManager* pServerMgr = GetServerManager();
    AdminServer* pAdminServer = GetAdminServer();
    Context* pContext = GetContext();

    uint64 windowStart = Time::GetTimeNowUS();
    uint64 busyUS = 0;
    uint32 passes = 0;

    while (pContext->IsRunning())
    {
        uint64 workStart = Time::GetTimeNowUS();
        pGameServer->Update();
        pServerMgr->Update(false);
        pAdminServer->Update();
        busyUS += Time::GetTimeNowUS() - workStart;
        ++passes;

        pGameServer->WaitForNetwork();

        uint64 now = Time::GetTimeNowUS();
        uint64 windowUS = now - windowStart;
        if (windowUS >= 1000000)
        {
            pContext->GetRuntimeStats().GetPerfStats().netCpuPermille = Min((uint32)((busyUS * 1000) / windowUS), 1000);

            if (passes < 500 && passes > 350)
            {
                LOGGER_LOG_WARN("[NET] event loop ran only %u passes, timer resolution problem?", passes);
            }

            passes = 0;
            busyUS = 0;
            windowStart = now;
        }
    }
}

void RunGameLoop()
{
    Context* pContext = GetContext();
    GameServer* pGameServer = GetGameServer();
    ServerManager* pServerMgr = GetServerManager();
    AdminCommandManager* pAdminCmdManager = GetAdminCommandManager();

    uint64 now = Time::GetSystemTime();
    uint64 nextTick = now + GAME_TICK_MS;

    uint64 lastPerfUpdateTime = now;
    uint64 tickDurSum = 0;
    uint32 tickCount = 0;

    uint32 intervalMaxTickMs = 0;
    uint32 intervalMaxLagSpikeMs = 0;
    uint64 totalWorkTimeInInterval = 0;

    while (pContext->IsRunning())
    {
        uint64 loopIterStart = Time::GetSystemTime();
        uint64 loopStartUS = Time::GetTimeNowUS();
        now = loopIterStart;
        uint32 loops = 0;

        pServerMgr->UpdateServers();
        pServerMgr->UpdateTCPLogic(NETWORK_BUDGET_MS);
        GetStatsManager()->Update(pContext->GetDatabasePool());
        GetHolidayManager()->Update();
        ProcessDatabaseResults(DB_RESULT_BUDGET_MS);
        pAdminCmdManager->ProcessTimeouts();

        if (pContext->IsShutting())
        {
            if (!firstCallShutdown)
                ForceSaveEverything();
            nextTick = now + GAME_TICK_MS;
            SleepMS(1);
            continue;
        }

        while (now >= nextTick && loops < MAX_CATCHUP_TICKS)
        {
            uint64 tickStartUS = Time::GetTimeNowUS();
            pGameServer->UpdateGameLogic(GAME_TICK_MS);

            uint64 tickUS = Time::GetTimeNowUS() - tickStartUS;
            uint32 tickDur = (uint32)(tickUS / 1000);

            tickDurSum += tickDur;
            ++tickCount;

            intervalMaxTickMs = Max(intervalMaxTickMs, tickDur);
            if (tickDur > GAME_TICK_MS)
            {
                intervalMaxLagSpikeMs = Max(intervalMaxLagSpikeMs, tickDur - GAME_TICK_MS);
            }

            nextTick += GAME_TICK_MS;
            ++loops;
            now = Time::GetSystemTime();

            if (pContext->IsShutting() && !firstCallShutdown)
            {
                ForceSaveEverything();
            }
        }

        if (now >= nextTick)
            nextTick = now + GAME_TICK_MS;

        uint64 loopWorkUS = Time::GetTimeNowUS() - loopStartUS;
        totalWorkTimeInInterval += loopWorkUS;

        if (now - lastPerfUpdateTime >= PERF_SAMPLE_INTERVAL_MS)
        {
            ContextPerfStats& perf = pContext->GetRuntimeStats().GetPerfStats();
            uint32 elapsedIntervalMs = (uint32)(now - lastPerfUpdateTime);

            perf.avgTickMs = (tickCount > 0) ? (uint32)(tickDurSum / tickCount) : 0;
            perf.maxTickMs = intervalMaxTickMs;
            perf.lagSpikeMs = intervalMaxLagSpikeMs;
            perf.cpuPermille = Min((uint32)((totalWorkTimeInInterval * 1000) / elapsedIntervalMs), 1000u);

            tickDurSum = 0;
            tickCount = 0;
            intervalMaxTickMs = 0;
            intervalMaxLagSpikeMs = 0;
            totalWorkTimeInInterval = 0;
            lastPerfUpdateTime = now;
        }

        if (nextTick > now)
        {
            SleepMS((uint32)(nextTick - now));
        }
    }
}

int main(int argc, char const* argv[])
{
#ifdef _WIN32
    timeBeginPeriod(1);
#endif

    SystemSignal::RegisterShutdownHook(GetContext()->GetShutdownFlag());

    if (!GetLog()->InitFile(GetProgramPath() + "/logs/log_MASTER.txt"))
    {
        LOGGER_LOG_ERROR_FLUSH("Failed to init log file, maybe try to create 'logs' folder?");
        return 0;
    }

    LOGGER_LOG_INFO_FLUSH("Starting Master Server");
    LOGGER_LOG_INFO_FLUSH("Project created by keichira https://github.com/keichira/GTopia");

    GetContext()->Init();
    SetRandomSeed(Time::GetSystemTime());
    RandomizeRandomSeed();
    GetContext()->SetID(0);

    auto pGameConfig = GetContext()->GetGameConfig();
    if (pGameConfig->LoadServersMaster(GetProgramPath() + "/servers.txt") == 0)
    {
        LOGGER_LOG_ERROR_FLUSH("Failed to load servers.txt");
        GetContext()->Kill();
        return 0;
    }
    LOGGER_LOG_INFO_FLUSH("Loaded %d servers from servers.txt", pGameConfig->servers.size());

    if (!pGameConfig->LoadConfig(GetProgramPath() + "/config.txt"))
    {
        LOGGER_LOG_ERROR_FLUSH("Failed to load config.txt");
        GetContext()->Kill();
        return 0;
    }

    if (!GetRoleManager()->Load(GetProgramPath() + "/roles.txt"))
    {
        LOGGER_LOG_ERROR_FLUSH("Failed to load roles.txt");
        GetContext()->Kill();
        return 0;
    }

    if (!GetItemInfoManager()->Load(GetProgramPath() + "/items.txt"))
    {
        LOGGER_LOG_ERROR_FLUSH("Failed to load items.txt");
        GetContext()->Kill();
        return 0;
    }

    if (!GetPlayModManager()->Load(GetProgramPath() + "/playmods.txt"))
    {
        LOGGER_LOG_ERROR_FLUSH("Failed to load playmods.txt");
        GetContext()->Kill();
        return 0;
    }

    DatabaseConnectConfig dbConfig;
    dbConfig.host = pGameConfig->database.host.c_str();
    dbConfig.user = pGameConfig->database.user.c_str();
    dbConfig.pass = pGameConfig->database.pass.c_str();
    dbConfig.database = pGameConfig->database.database.c_str();
    dbConfig.port = pGameConfig->database.port;

    if (!GetContext()->GetDatabasePool()->Init(1, dbConfig))
    {
        LOGGER_LOG_ERROR_FLUSH(
            "Failed to initialize database pool, database credentials might be wrong check config.txt");
        GetContext()->Kill();
        return 0;
    }
    LOGGER_LOG_INFO_FLUSH("Loaded %d workers for database", GetContext()->GetDatabasePool()->GetWorkerSize());

    if (!GetContext()->LoadDatabaseModules())
        return 0;

    auto masterServerInfo = pGameConfig->servers[0];
    if (!GetServerManager()->Init(masterServerInfo.lanIP, masterServerInfo.tcpPort))
    {
        LOGGER_LOG_ERROR_FLUSH("Failed to initialize netsocket on %s:%d", masterServerInfo.lanIP.c_str(),
                               masterServerInfo.tcpPort);
        GetContext()->Kill();
        return 0;
    }
    LOGGER_LOG_INFO_FLUSH("Started netsocket on %s:%d", masterServerInfo.lanIP.c_str(), masterServerInfo.tcpPort);

    GetStatsManager()->SetSendTCPPacketCallback([](TCPPacketWriter& writer)
                                                { GetServerManager()->SendPacketToAll(writer); });

    GetHolidayManager()->SetSendTCPPacketCallback([](TCPPacketWriter& writer)
                                                  { GetServerManager()->SendPacketToAll(writer); });
    GetHolidayManager()->SetGameConfig(GetContext()->GetGameConfig());
    GetHolidayManager()->SetDatabasePool(GetContext()->GetDatabasePool());

    GetWorldManager()->SetBalancerGameConfig(GetContext()->GetGameConfig());
    GetWorldManager()->RegisterBalancedWorldsFromConfig();

    if (!GetGameServer()->Init(masterServerInfo.wanIP, masterServerInfo.udpPort))
    {
        LOGGER_LOG_ERROR_FLUSH("Failed to initialize game server on %s:%d", masterServerInfo.wanIP.c_str(),
                               masterServerInfo.udpPort);
        GetContext()->Kill();
        return 0;
    }
    GetGameServer()->SetENetIncomeCmdType(pGameConfig->enetIncomeCmdType);
    LOGGER_LOG_INFO_FLUSH("Started game server on %s:%d", masterServerInfo.wanIP.c_str(), masterServerInfo.udpPort);

    AdminServer* pAdminServer = GetAdminServer();
    if (pAdminServer->LoadConfigFromFile(GetProgramPath() + "/admin_server_config.txt"))
    {
        if (pAdminServer->IsEnabled())
        {
            if (!pAdminServer->Init())
            {
                LOGGER_LOG_ERROR_FLUSH("Failed to initialize admin server on %s:%d", pAdminServer->GetHost().c_str(),
                                       pAdminServer->GetPort());
                GetContext()->Kill();
                return 0;
            }
            LOGGER_LOG_INFO_FLUSH("Started admin server on %s:%d", pAdminServer->GetHost().c_str(),
                                  pAdminServer->GetPort());
        }
        else
        {
            LOGGER_LOG_INFO_FLUSH("Not starting admin server its disabled");
        }
    }
    else
    {
        LOGGER_LOG_ERROR_FLUSH("Failed to load admin_server_config.txt not gonna initialize admin server");
    }

    std::thread dbThread(DatabaseThreadFunc);
    std::thread eventThread(EventThreadFunc);

    RunGameLoop();

    LOGGER_LOG_INFO_FLUSH("Killing Master server");
    GetLog()->Flush();
    GetContext()->GetDatabasePool()->GetWorker(0)->SendFakeTask();

    if (dbThread.joinable())
        dbThread.join();
    if (eventThread.joinable())
        eventThread.join();

    GetAdminServer()->Kill();
    GetGameServer()->Kill();
    GetServerManager()->Kill();

    GetLog()->Kill();
    GetContext()->Kill();

    mysql_library_end();
    SystemSignal::SignalShutdownComplete();

#ifdef _WIN32
    timeEndPeriod(1);
#endif
    return 0;
}