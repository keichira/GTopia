#include "Context.h"
#include "IO/Log.h"
#include "Math/Random.h"
#include "Utils/Timer.h"
#include "Server/MasterBroadway.h"
#include "Utils/StringUtils.h"
#include "Item/ItemInfoManager.h"
#include "IO/File.h"
#include "Server/GameServer.h"
#include "Server/MasterBroadway.h"
#include "Player/PlayerTribute.h"
#include "World/WorldManager.h"
#include "Player/RoleManager.h"
#include "Player/PlayModManager.h"
#include "Math/Math.h"
#include "Player/AchievementManager.h"
#include "Store/StoreManager.h"
#include "Server/UserCacheManager.h"

bool firstCallShutdown = false;

#include <signal.h>
void SignalStop(int32 signum) 
{
    GetContext()->Shutdown();
    //GetContext()->Stop();
}

void ForceSaveEverything()
{
    firstCallShutdown = true;

    GetMasterBroadway()->SendServerKillPacket();
    GetGameServer()->ForceSaveEverything();

    QueryRequest req;
    req.callback = [](QueryTaskResult&&) { GetContext()->Stop(); };
    DatabaseExec(GetContext()->GetDatabasePool(), "", req, QUERY_FLAG_NONE);
}

bool ReadArgs(int argc, char const* argv[]) 
{
    bool idSet = false;

    for(int i = 1; i < argc; ++i) {
        if(string(argv[i]) == "--id") {
            uint16 id = ToUInt(argv[i+1]);
            if(id <= 0) {
                LOGGER_LOG_ERROR("server id must bigger than 0");
                return false;
            }

            GetContext()->SetID(id);
            idSet = true;
        }
    }

    if(!idSet) {
        LOGGER_LOG_ERROR("No --id param detected it must set!");
        return false;
    }

    return true;
}

void DatabaseThreadFunc()
{
    Timer logTimer;

    uint64 now = Time::GetSystemTime();
    DatabaseWorker* pWorker = GetContext()->GetDatabasePool()->GetWorker(0);

    while(GetContext()->IsRunning()) {
        now = Time::GetSystemTime();

        pWorker->Update();

        if(logTimer.GetElapsedTime() >= 5000) 
        {
            GetLog()->Write();
            logTimer.Reset();
        }

        SleepMS(1);
    }
}

void EventThreadFunc() {
    Context* pContext = GetContext();
    GameServer* pGameServer = GetGameServer();
    MasterBroadway* pMaster = GetMasterBroadway();

    uint64 lastCalculateTime = Time::GetSystemTime();
    uint64 totalWorkTime = 0;

    while(pContext->IsRunning()) 
    {
        uint64 currentTime = Time::GetSystemTime();

        uint32 elapsedMs = (uint32)(currentTime - lastCalculateTime);
        if (elapsedMs >= 1000) {
            uint32 permille = (uint32)((totalWorkTime * 1000) / elapsedMs);

            if(permille > 1000)
            {
                permille = 1000;
            }

            pContext->GetPerfStats().netCpuPermille = permille;

            totalWorkTime = 0;
            lastCalculateTime = currentTime;
        }

        uint64 workStart = Time::GetSystemTime();

        pGameServer->Update();
        pMaster->Update(true);

        uint64 workEnd = Time::GetSystemTime();
        totalWorkTime += (workEnd - workStart);

        SleepMS(1);
    }
}

void ProcessDatabaseResults(uint64 maxTimeMS)
{
    DatabasePool* pDatabasePool = GetContext()->GetDatabasePool();
    if(!pDatabasePool) {
        return;
    }

    uint64 startTime = Time::GetSystemTime();

    QueryTaskResult taskRes;

    while(pDatabasePool->GetResult(taskRes)) {
        if(taskRes.callback) {
            taskRes.callback(std::move(taskRes));
        }

        taskRes.Destroy();

        if(Time::GetSystemTime() - startTime >= maxTimeMS) {
            break;
        }
    }
}

bool LoadItemData()
{
    ItemInfoManager* pItemMgr = GetItemInfoManager();
    GameConfig* pGameConfig = GetContext()->GetGameConfig();

    if(pGameConfig->forceItemDataVersion != 0) {
        pItemMgr->ForceItemDataVersion(pGameConfig->forceItemDataVersion);
    }

    if(!pItemMgr->Load(GetProgramPath() + "/items.txt")) {
        LOGGER_LOG_ERROR("Failed to load items.txt");
        return false;
    }

    if(!pItemMgr->LoadWikiData(GetProgramPath() + "/wiki_data.txt")) {
        LOGGER_LOG_ERROR("wiki_data.txt not found, skipping");
    }
    pItemMgr->SetupItemExtras();

    bool usingGTCDN = pGameConfig->cdnServer.find("ubistatic-a.akamaihd.net") != string::npos;
    if(usingGTCDN)
    {
        LOGGER_LOG_WARN("Detected ubistatic-a.akamaihd.net skipping filehashes.txt");
    }

    std::unordered_map<string, uint32> hashData;
    if(!usingGTCDN)
    {
        File fileHashes;
        if(!fileHashes.Open(GetProgramPath() + "/filehashes.txt")) {
            LOGGER_LOG_ERROR("Failed to load filehashes.txt");
            return false;
        }
    
        uint32 fileSize = fileHashes.GetSize();
        string fileData(fileSize, '\0');
    
        if(fileHashes.Read(fileData.data(), fileSize) != fileSize) {
            fileHashes.Close();
            return false;
        }
        auto lines = Split(fileData, '\n');
        fileHashes.Close();
    
        for(auto& line : lines) {
            if(line.empty()) {
                continue;
            }
    
            auto args = Split(line, '|');
            hashData.insert_or_assign(args[0], ToUInt(args[1]));
        }
    }

    uint32 minVersion = GetMinRequiredItemDataVersion(
        pGameConfig->androidSupportedVersions[0], pGameConfig->windowsSupportedVersions[0],
        pGameConfig->iosSupportedVersions[0], pGameConfig->macosSupportedVersions[0]
    );

    uint32 maxVersion = GetMinRequiredItemDataVersion(
        pGameConfig->androidSupportedVersions[1], pGameConfig->windowsSupportedVersions[1],
        pGameConfig->iosSupportedVersions[1], pGameConfig->macosSupportedVersions[1]
    );

    if(!usingGTCDN)
    {
        pItemMgr->LoadFileHashes(hashData, false);
    }
    pItemMgr->SaveToClientData(false, minVersion, maxVersion);

    if(!usingGTCDN)
    {
        pItemMgr->LoadFileHashes(hashData, true);
    }
    pItemMgr->SaveToClientData(true, minVersion, maxVersion);

    if(!pItemMgr->LoadConsumableData(GetProgramPath() + "/consumable_data.txt"))
    {
        LOGGER_LOG_ERROR("consumable_data.txt not found skipping");
    }

    if(!pItemMgr->LoadBattlePetData(GetProgramPath() + "/battle_pet_data.txt"))
    {
        LOGGER_LOG_ERROR("battle_pet_data.txt not found skipping");
    }

    /*PlayerTribute* pPlayerTrib = GetPlayerTributeManager();
    if(!pPlayerTrib->Load(GetProgramPath() + "/player_tribute.txt")) {
        LOGGER_LOG_ERROR("Failed to load player_tribute.txt anyways skipping it");
    }
    else {
        pPlayerTrib->SaveToClientData();
    }*/

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
    uint64 loopIterStart = now;

    while(pContext->IsRunning())
    {
        loopIterStart = Time::GetSystemTime();
        now = loopIterStart;
        uint32 loops = 0;

        pMaster->UpdateTCPLogic(NETWORK_BUDGET_MS);
        ProcessDatabaseResults(DB_RESULT_BUDGET_MS);

        if(pContext->IsShutting())
        {
            if(!firstCallShutdown)
            {
                ForceSaveEverything();
            }

            nextTick = now + GAME_TICK_MS;
            
            SleepMS(1);
            continue;
        }

        while(now >= nextTick && loops < MAX_CATCHUP_TICKS)
        {
            uint64 tickStart = Time::GetSystemTime();

            pGameServer->UpdateGameLogic(GAME_TICK_MS);

            uint64 tickEnd = Time::GetSystemTime();
            uint32 tickDur = (uint32)(tickEnd - tickStart);

            tickDurSum += tickDur;
            ++tickCount;

            intervalMaxTickMs = Max(intervalMaxTickMs, tickDur);
            if(tickDur > GAME_TICK_MS)
            {
                uint32 currentSpike = tickDur - GAME_TICK_MS;
                intervalMaxLagSpikeMs = Max(intervalMaxLagSpikeMs, currentSpike);
            }

            nextTick += GAME_TICK_MS;
            ++loops;
            now = Time::GetSystemTime();

            if(pContext->IsShutting() && !firstCallShutdown)
            {
                ForceSaveEverything();
            }
        }

        if(now >= nextTick)
        {
            nextTick = now + GAME_TICK_MS;
        }

        uint64 loopIterEnd = Time::GetSystemTime();
        totalWorkTimeInInterval += (loopIterEnd - loopIterStart);

        if(now - lastPerfUpdateTime >= PERF_SAMPLE_INTERVAL_MS)
        {
            ContextPerfStats& perf = pContext->GetPerfStats();
            uint32 elapsedIntervalMs = (uint32)(now - lastPerfUpdateTime);

            if(tickCount > 0)
            {
                perf.avgTickMs = (uint32)(tickDurSum / tickCount);
            }
            else
            {
                perf.avgTickMs = 0;
            }

            perf.maxTickMs = intervalMaxTickMs;
            perf.lagSpikeMs = intervalMaxLagSpikeMs;

            perf.cpuPermille = (uint32)((totalWorkTimeInInterval * 1000) / elapsedIntervalMs);
            if(perf.cpuPermille > 1000)
            {
                perf.cpuPermille = 1000;
            }

            tickDurSum = 0;
            tickCount = 0;
            intervalMaxTickMs = 0;
            intervalMaxLagSpikeMs = 0;
            totalWorkTimeInInterval = 0;
            lastPerfUpdateTime = now;
        }

        if(nextTick > now)
        {
            SleepMS((uint32)(nextTick - now));
        }
    }
}

int main(int argc, char const* argv[])
{
    signal(SIGTERM, SignalStop);
    signal(SIGINT, SignalStop);
    signal(SIGSEGV, SignalStop);
    signal(SIGABRT, SignalStop);

    if(!ReadArgs(argc, argv))
        return 0;

    LOGGER_LOG_INFO("Starting Game Server %d", GetContext()->GetID());
    LOGGER_LOG_INFO("Project created by keichira https://github.com/keichira/GTopia")

    GetContext()->Init();
    
    SetRandomSeed(Time::GetSystemTime());
    RandomizeRandomSeed();
    
    if(!GetLog()->InitFile(GetProgramPath() + "/logs/log_SERVER_" + ToString(GetContext()->GetID()) + ".txt")) 
    {
        LOGGER_LOG_ERROR("Failed to init log file, maybe try to create 'logs' folder?");
        return 0;
    }

    auto pGameConfig = GetContext()->GetGameConfig();
    if(pGameConfig->LoadServersClient(GetProgramPath() + "/servers.txt", GetContext()->GetID()) != 2) 
    {
        LOGGER_LOG_ERROR("Failed to load servers.txt");
        return 0;
    }

    if(pGameConfig->servers[1].serverType != CONFIG_SERVER_GAME) 
    {
        LOGGER_LOG_ERROR("Woops trying to run server with wrong type %d it should be game", pGameConfig->servers[1].serverType);
        return 0;
    }

    if(!pGameConfig->LoadConfig(GetProgramPath() + "/config.txt")) 
    {
        LOGGER_LOG_ERROR("Failed to load config.txt");
        return 0;
    }

    auto gameServerInfo = pGameConfig->servers[1];
    if(!GetMasterBroadway()->Init(gameServerInfo.lanIP, gameServerInfo.tcpPort, 0)) 
    {
        LOGGER_LOG_ERROR("Failed to initialize netsocket on %s:%d", gameServerInfo.lanIP.c_str(), gameServerInfo.tcpPort);
        return 0;
    }
    LOGGER_LOG_INFO("Started netsocket on %s:%d", gameServerInfo.lanIP.c_str(), gameServerInfo.tcpPort);

    LOGGER_LOG_INFO("Connecting to master server");
    auto masterServerInfo = pGameConfig->servers[0];

    while(!GetContext()->IsShutting()) 
    {
        if(GetMasterBroadway()->Connect(masterServerInfo.lanIP, masterServerInfo.tcpPort, 5, GetContext()->GetShutdownFlag())) 
        {
            break;
        }
        else 
        {
            LOGGER_LOG_ERROR("Failed to connect master server");
            return 0;
        }
    }

    LOGGER_LOG_INFO("Connected to master server");

    if(!GetRoleManager()->Load(GetProgramPath() + "/roles.txt")) 
    {
        LOGGER_LOG_ERROR("Failed to load roles.txt");
        return 0;
    }

    if(!LoadItemData()) 
        return 0;

    if(!GetPlayModManager()->Load(GetProgramPath() + "/playmods.txt")) 
    {
        LOGGER_LOG_ERROR("Failed to load playmods.txt");
    }

    if(!GetAchievementManager()->Load(GetProgramPath() + "/achievements.txt"))
    {
        LOGGER_LOG_ERROR("Failed to load achievements.txt");
    }

    if(!GetStoreManager()->Load(GetProgramPath() + "/store.txt"))
    {
        LOGGER_LOG_ERROR("Failed to load store.txt");
    }

    GetMasterBroadway()->SendHelloPacket();

    DatabaseConnectConfig dbConfig;
    dbConfig.host = pGameConfig->database.host.c_str();
    dbConfig.user = pGameConfig->database.user.c_str();
    dbConfig.pass = pGameConfig->database.pass.c_str();
    dbConfig.database = pGameConfig->database.database.c_str();
    dbConfig.port = pGameConfig->database.port;

    if(!GetContext()->GetDatabasePool()->Init(1, dbConfig)) 
    {
        LOGGER_LOG_ERROR("Failed to initialize database pool");
        return 0;
    }
    LOGGER_LOG_INFO("Loaded %d workers for database", GetContext()->GetDatabasePool()->GetWorkerSize());

    if(!GetGameServer()->Init(gameServerInfo.wanIP, gameServerInfo.udpPort))
    {
        LOGGER_LOG_ERROR("Failed to initialize game server on %s:%d", gameServerInfo.wanIP.c_str(), gameServerInfo.udpPort);
        return 0;
    }
    GetGameServer()->SetENetIncomeCmdType(pGameConfig->enetIncomeCmdType);
    LOGGER_LOG_INFO("Started game server on %s:%d", gameServerInfo.wanIP.c_str(), gameServerInfo.udpPort);

    GetUserCacheManager()->Init(100);

    gNetBurstConfig.threshold.heavyQueueSize = pGameConfig->netThreshold.heavyQueueSize;
    gNetBurstConfig.threshold.panicQueueSize = pGameConfig->netThreshold.panicQueueSize;
    gNetBurstConfig.threshold.heavyCpuPermille = pGameConfig->netThreshold.heavyCpuPermille;
    gNetBurstConfig.threshold.panicCpuPermille = pGameConfig->netThreshold.panicBurst;
    gNetBurstConfig.normalBurst = pGameConfig->netThreshold.normalBurst;
    gNetBurstConfig.heavyBurst = pGameConfig->netThreshold.heavyBurst;
    gNetBurstConfig.panicBurst = pGameConfig->netThreshold.panicBurst;

    std::thread dbThread(DatabaseThreadFunc);
    std::thread eventThread(EventThreadFunc);

    RunGameLoop();

    LOGGER_LOG_ERROR("Killing Game server %d", GetContext()->GetID());

    GetLog()->Flush();

    if(dbThread.joinable()) dbThread.join();
    if(eventThread.joinable()) eventThread.join();

    GetGameServer()->Kill();
    GetMasterBroadway()->Kill();
    GetContext()->Kill();

    GetLog()->Kill();

    mysql_library_end();
    return 0;
}
