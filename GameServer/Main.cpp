#include "Context.h"
#include "IO/File.h"
#include "IO/Log.h"
#include "Item/ItemInfoManager.h"
#include "Math/Math.h"
#include "Math/Random.h"
#include "Network/DiscordWebhook.h"
#include "Player/AchievementManager.h"
#include "Player/PlayModManager.h"
#include "Player/PlayerTribute.h"
#include "Player/RoleManager.h"
#include "Server/GamePresenceManager.h"
#include "Server/GameServer.h"
#include "Server/MasterBroadway.h"
#include "Server/StatsManager.h"
#include "Server/UserCacheManager.h"
#include "Store/StoreManager.h"
#include "Utils/StringUtils.h"
#include "Utils/Timer.h"
#include "World/WorldManager.h"

// clang-format off
#ifdef _WIN32
#include <windows.h>
#include <timeapi.h>
#endif
// clang-format on

#ifdef ENABLE_LUA_SCRIPTING
#include "Lua/LuaManager.h"
#endif

bool firstCallShutdown = false;

void ForceSaveEverything()
{
    firstCallShutdown = true;

    GetMasterBroadway()->SendServerKillPacket();
    GetGameServer()->ForceSaveEverything();

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

bool ReadArgs(int argc, char const* argv[])
{
    bool idSet = false;

    for (int32 i = 1; i < argc; ++i)
    {
        if (string(argv[i]) == "--id")
        {
            uint16 id = ToUInt(argv[i + 1]);
            if (id <= 0)
            {
                LOGGER_LOG_ERROR_FLUSH("server id must bigger than 0");
                return false;
            }

            GetContext()->SetID(id);
            idSet = true;
        }
    }

    if (!idSet)
    {
        LOGGER_LOG_ERROR_FLUSH("No --id param detected it must set!");
        return false;
    }

    return true;
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
    Context* pContext = GetContext();
    GameServer* pGameServer = GetGameServer();
    MasterBroadway* pMaster = GetMasterBroadway();

    uint64 windowStart = Time::GetTimeNowUS();
    uint64 busyUS = 0;
    uint32 passes = 0;

    while (pContext->IsRunning())
    {
        uint64 workStart = Time::GetTimeNowUS();
        pGameServer->Update();
        pMaster->Update(true);
        busyUS += Time::GetTimeNowUS() - workStart;
        ++passes;

        pGameServer->WaitForNetwork();

        uint64 now = Time::GetTimeNowUS();
        uint64 windowUS = now - windowStart;
        if (windowUS >= 1000000)
        {
            pContext->GetRuntimeStats().GetPerfStats().netCpuPermille =
                Min((uint32)((busyUS * 1000) / windowUS), 1000u);

            if (passes < 500 && passes > 350) // reduce the log
            {
                LOGGER_LOG_WARN("[NET] event loop ran only %u passes, timer resolution problem?", passes);
            }

            passes = 0;
            busyUS = 0;
            windowStart = now;
        }
    }
}

void ProcessDatabaseResults(uint64 maxTimeMS)
{
    DatabasePool* pDatabasePool = GetContext()->GetDatabasePool();
    if (!pDatabasePool)
        return;

    uint64 startTime = Time::GetSystemTime();
    QueryTaskResult taskRes;

    while (pDatabasePool->GetResult(taskRes))
    {
        if (taskRes.callback)
        {
            taskRes.callback(std::move(taskRes));
        }

        taskRes.Destroy();

        if (Time::GetSystemTime() - startTime >= maxTimeMS)
            break;
    }
}

bool LoadItemData()
{
    ItemInfoManager* pItemMgr = GetItemInfoManager();
    GameConfig* pGameConfig = GetContext()->GetGameConfig();

    if (pGameConfig->forceItemDataVersion != 0)
    {
        pItemMgr->ForceItemDataVersion(pGameConfig->forceItemDataVersion);
    }

    if (!pItemMgr->Load(GetProgramPath() + "/items.txt"))
    {
        LOGGER_LOG_ERROR_FLUSH("Failed to load items.txt");
        return false;
    }

    if (!pItemMgr->LoadWikiData(GetProgramPath() + "/wiki_data.txt"))
    {
        LOGGER_LOG_ERROR_FLUSH("wiki_data.txt not found, skipping");
    }
    pItemMgr->SetupItemExtras();

    bool usingGTCDN = pGameConfig->cdnServer.find("ubistatic-a.akamaihd.net") != string::npos;
    if (usingGTCDN)
    {
        LOGGER_LOG_WARN_FLUSH("Detected ubistatic-a.akamaihd.net skipping filehashes.txt");
    }

    std::unordered_map<string, uint32> hashData;
    if (!usingGTCDN)
    {
        File fileHashes;
        if (!fileHashes.Open(GetProgramPath() + "/filehashes.txt"))
        {
            LOGGER_LOG_ERROR_FLUSH("Failed to load filehashes.txt");
            return false;
        }

        uint32 fileSize = fileHashes.GetSize();
        string fileData(fileSize, '\0');

        if (fileHashes.Read(fileData.data(), fileSize) != fileSize)
        {
            fileHashes.Close();
            return false;
        }
        auto lines = Split(fileData, '\n');
        fileHashes.Close();

        for (auto& line : lines)
        {
            if (line.empty())
                continue;
            auto args = Split(line, '|');
            if (args.size() >= 2)
                hashData.insert_or_assign(args[0], ToUInt(args[1]));
        }
    }

    uint32 minVersion = GetMinRequiredItemDataVersion(
        pGameConfig->androidSupportedVersions[0], pGameConfig->windowsSupportedVersions[0],
        pGameConfig->iosSupportedVersions[0], pGameConfig->macosSupportedVersions[0]);

    uint32 maxVersion = GetMinRequiredItemDataVersion(
        pGameConfig->androidSupportedVersions[1], pGameConfig->windowsSupportedVersions[1],
        pGameConfig->iosSupportedVersions[1], pGameConfig->macosSupportedVersions[1]);

    bool allSupportZLib =
        (pGameConfig->androidSupportedVersions[0] >= 3.02f) && (pGameConfig->windowsSupportedVersions[0] >= 3.02f) &&
        (pGameConfig->iosSupportedVersions[0] >= 3.02f) && (pGameConfig->macosSupportedVersions[0] >= 3.02f);

    uint32 totalSavedSize = 0;
    LOGGER_LOG_INFO("Serializing item data into memory");

    if (!usingGTCDN)
        pItemMgr->LoadFileHashes(hashData, false);
    pItemMgr->SaveToClientData(false, minVersion, maxVersion, allSupportZLib, totalSavedSize);

    if (!usingGTCDN)
        pItemMgr->LoadFileHashes(hashData, true);
    pItemMgr->SaveToClientData(true, minVersion, maxVersion, allSupportZLib, totalSavedSize);

    LOGGER_LOG_INFO("Items data totally takes %d bytes from memory", totalSavedSize);

    if (!pItemMgr->LoadConsumableData(GetProgramPath() + "/consumable_data.txt"))
    {
        LOGGER_LOG_ERROR_FLUSH("consumable_data.txt not found skipping");
    }

    return true;
}

void RunGameLoop()
{
    Context* pContext = GetContext();
    GameServer* pGameServer = GetGameServer();
    MasterBroadway* pMaster = GetMasterBroadway();

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

        pMaster->UpdateTCPLogic(NETWORK_BUDGET_MS);
        ProcessDatabaseResults(DB_RESULT_BUDGET_MS);

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

            GetUserCacheManager()->Update();
            GetGamePresenceManager()->Update();

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
            perf.cpuPermille =
                Min((uint32)((totalWorkTimeInInterval * 1000) / ((uint64)elapsedIntervalMs * 1000)), 1000u);

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
    // yea because of windows things
    // but still im not sure about it
    timeBeginPeriod(1);
#endif

    SystemSignal::RegisterShutdownHook(GetContext()->GetShutdownFlag());

    if (!ReadArgs(argc, argv))
        return 0;

    if (!GetLog()->InitFile(GetProgramPath() + "/logs/log_SERVER_" + ToString(GetContext()->GetID()) + ".txt"))
    {
        LOGGER_LOG_ERROR_FLUSH("Failed to init log file, maybe try to create 'logs' folder?");
        return 0;
    }

    LOGGER_LOG_INFO_FLUSH("Starting Game Server %d", GetContext()->GetID());
    LOGGER_LOG_INFO_FLUSH("Project created by keichira https://github.com/keichira/GTopia");

    GetContext()->Init();
    SetRandomSeed(Time::GetSystemTime());
    RandomizeRandomSeed();

    auto pGameConfig = GetContext()->GetGameConfig();
    if (pGameConfig->LoadServersClient(GetProgramPath() + "/servers.txt", GetContext()->GetID()) != 2)
    {
        LOGGER_LOG_ERROR_FLUSH("Failed to load servers.txt");
        GetContext()->Kill();
        return 0;
    }

    if (pGameConfig->servers[1].serverType != CONFIG_SERVER_GAME)
    {
        LOGGER_LOG_ERROR_FLUSH("Woops trying to run server with wrong type %d it should be game",
                               pGameConfig->servers[1].serverType);
        GetContext()->Kill();
        return 0;
    }

    if (!pGameConfig->LoadConfig(GetProgramPath() + "/config.txt"))
    {
        LOGGER_LOG_ERROR_FLUSH("Failed to load config.txt");
        GetContext()->Kill();
        return 0;
    }

    GetDiscordWebhookManager()->SetGameConfig(GetContext()->GetGameConfig());
    GetDiscordWebhookManager()->RegisterFromConfig(CONFIG_SERVER_GAME);
    LOGGER_LOG_INFO("Registered %d Discord webhooks.", GetDiscordWebhookManager()->GetWebhookCount());

#ifdef ENABLE_LUA_SCRIPTING
    if (!GetLuaManager()->Initialize())
    {
        LOGGER_LOG_ERROR_FLUSH("Failed to initialize lua manager");
        return 0;
    }
    LOGGER_LOG_INFO("Lua manager initialized, loading lua scripts from '%s'", pGameConfig->luaScriptsPath.c_str());
    GetLuaManager()->LoadScriptsFromDirectory(pGameConfig->luaScriptsPath);
#endif

    if (!GetRoleManager()->Load(GetProgramPath() + "/roles.txt"))
    {
        LOGGER_LOG_ERROR_FLUSH("Failed to load roles.txt");
        GetContext()->Kill();
        return 0;
    }

    if (!LoadItemData())
    {
        GetContext()->Kill();
        return 0;
    }

    if (!GetPlayModManager()->Load(GetProgramPath() + "/playmods.txt"))
        LOGGER_LOG_ERROR_FLUSH("Failed to load playmods.txt");
    if (!GetAchievementManager()->Load(GetProgramPath() + "/achievements.txt"))
        LOGGER_LOG_ERROR_FLUSH("Failed to load achievements.txt");
    if (!GetStoreManager()->Load(GetProgramPath() + "/store.txt"))
        LOGGER_LOG_ERROR_FLUSH("Failed to load store.txt");
    if (!pGameConfig->LoadNews(GetProgramPath() + "/news.txt"))
        LOGGER_LOG_ERROR_FLUSH("Failed to load news.txt");

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

    auto gameServerInfo = pGameConfig->servers[1];
    if (!GetMasterBroadway()->Init(gameServerInfo.lanIP, gameServerInfo.tcpPort, 0))
    {
        LOGGER_LOG_ERROR_FLUSH("Failed to initialize netsocket on %s:%d", gameServerInfo.lanIP.c_str(),
                               gameServerInfo.tcpPort);
        GetContext()->Kill();
        return 0;
    }
    LOGGER_LOG_INFO_FLUSH("Started netsocket on %s:%d", gameServerInfo.lanIP.c_str(), gameServerInfo.tcpPort);

    GetStatsManager()->SetSendTCPPacketCallback([](TCPPacketWriter& writer)
                                                { GetMasterBroadway()->SendPacketRaw(writer); });

    GetWorldManager()->SetBalancerGameConfig(GetContext()->GetGameConfig());
    GetWorldManager()->RegisterBalancedWorldsFromConfig();

    if (!GetGameServer()->Init(gameServerInfo.wanIP, gameServerInfo.udpPort))
    {
        LOGGER_LOG_ERROR_FLUSH("Failed to initialize game server on %s:%d", gameServerInfo.wanIP.c_str(),
                               gameServerInfo.udpPort);
        GetContext()->Kill();
        return 0;
    }
    GetGameServer()->SetENetIncomeCmdType(pGameConfig->enetIncomeCmdType);
    LOGGER_LOG_INFO_FLUSH("Started game server on %s:%d", gameServerInfo.wanIP.c_str(), gameServerInfo.udpPort);

    GetUserCacheManager()->Init(100);

    auto masterServerInfo = pGameConfig->servers[0];
    LOGGER_LOG_INFO_FLUSH("Connecting and Authenticating with Master Server...");

    if (!GetMasterBroadway()->ConnectAndAuth(masterServerInfo.lanIP, masterServerInfo.tcpPort, 3,
                                             GetContext()->GetShutdownFlag()))
    {
        LOGGER_LOG_ERROR_FLUSH("Master Server connection or authentication failed. Aborting startup.");
        GetContext()->Kill();
        return 0;
    }

    std::thread dbThread(DatabaseThreadFunc);
    std::thread eventThread(EventThreadFunc);

    GetMasterBroadway()->SendReadyPacket();
    LOGGER_LOG_INFO_FLUSH("GameServer fully loaded. Sent ready packet to Master Server.");

    RunGameLoop();

    LOGGER_LOG_INFO("Killing Game server %d", GetContext()->GetID());
    GetLog()->Flush();
    GetContext()->GetDatabasePool()->GetWorker(0)->SendFakeTask();

    if (dbThread.joinable())
        dbThread.join();
    if (eventThread.joinable())
        eventThread.join();

    GetGameServer()->Kill();
    GetMasterBroadway()->Kill();
    GetDiscordWebhookManager()->Kill();
    GetContext()->Kill();
    GetLog()->Kill();

    mysql_library_end();
    SystemSignal::SignalShutdownComplete();

#ifdef _WIN32
    timeEndPeriod(1);
#endif
    return 0;
}