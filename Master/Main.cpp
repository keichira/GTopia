#include "Context.h"
#include "IO/Log.h"
#include "Math/Random.h"
#include "Utils/Timer.h"
#include "Server/GameServer.h"
#include "Server/ServerManager.h"
#include "Player/GamePlayer.h"
#include "World/WorldManager.h"
#include "Server/TelnetServer.h"
#include "Player/RoleManager.h"
#include "Math/Math.h"

#include <signal.h>
void SignalStop(int32 signum) 
{ 
    LOGGER_LOG_WARN("Received signal %d", signum);
    GetContext()->Shutdown();
}

void DatabaseThreadFunc() 
{
    uint64 lastLogWriteTime = Time::GetSystemTime();
    uint64 nextTick = Time::GetSystemTime();

    while(GetContext()->IsRunning()) {
        DatabaseWorker* pWorker = GetContext()->GetDatabasePool()->GetWorker(0);
        pWorker->Update();

        uint64 logWriteStart = Time::GetSystemTime();
        if(logWriteStart - lastLogWriteTime >= 5000) {
            GetLog()->Write();
            lastLogWriteTime = Time::GetSystemTime();
        }

        SleepMS(1);
    }
}

void EventThreadFunc() 
{
    GameServer* pGameServer = GetGameServer();
    ServerManager* pServerMgr = GetServerManager();
    TelnetServer* pTelnetServer = GetTelnetServer();
    Context* pContext = GetContext();

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
        pServerMgr->Update(false);
        pTelnetServer->Update(); // todo here handle on gameloop

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

    QueryTaskResult taskRes;
    Timer startTime;

    while(pDatabasePool->GetResult(taskRes)) {
        if(taskRes.callback) {
            taskRes.callback(std::move(taskRes));
            taskRes.Destroy();
        }

        if(startTime.GetElapsedTime() >= maxTimeMS) {
            break;
        }
    }
}

void RunGameLoop()
{
    Context* pContext = GetContext();
    GameServer* pGameServer = GetGameServer();
    ServerManager* pServerMgr = GetServerManager();

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

        pServerMgr->UpdateTCPLogic(NETWORK_BUDGET_MS);
        ProcessDatabaseResults(DB_RESULT_BUDGET_MS);

        if(pContext->IsShutting())
        {
            pContext->Stop();
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

/*void RegisterBalancedWorlds()
{
    GameConfig* pGameConfig = GetContext()->GetGameConfig();
    WorldManager* pWorldMgr = GetWorldManager();

    pWorldMgr->SetBalancerEnabled(pGameConfig->isWorldBalancerEnabled);
    if(!pWorldMgr->IsBalancerEnabled())
        return;

    for(auto& balance : pGameConfig->balancedWorlds)
    {
        pWorldMgr->RegisterBalancedWorld(balance);
    }
}*/

int main(int argc, char const* argv[])
{
    signal(SIGTERM, SignalStop);
    signal(SIGINT, SignalStop);

    LOGGER_LOG_INFO("Starting Master Server");
    LOGGER_LOG_INFO("Project created by keichira https://github.com/keichira/GTopia")
    
    GetContext()->Init();

    SetRandomSeed(Time::GetSystemTime());
    RandomizeRandomSeed();

    GetContext()->SetID(0);
    if(!GetLog()->InitFile(GetProgramPath() + "/logs/log_MASTER.txt")) {
        LOGGER_LOG_ERROR("Failed to init log file, maybe try to create 'logs' folder?");
        return 0;
    }

    auto pGameConfig = GetContext()->GetGameConfig();
    if(pGameConfig->LoadServersMaster(GetProgramPath() + "/servers.txt") == 0) {
        LOGGER_LOG_ERROR("Failed to load servers.txt");
        return 0;
    }
    LOGGER_LOG_INFO("Loaded %d servers from servers.txt", pGameConfig->servers.size());

    if(!pGameConfig->LoadConfig(GetProgramPath() + "/config.txt")) {
        LOGGER_LOG_ERROR("Failed to load config.txt");
        return 0;
    }

    if(!GetRoleManager()->Load(GetProgramPath() + "/roles.txt")) {
        LOGGER_LOG_ERROR("Failed to load roles.txt");
        return 0;
    }

    auto masterServerInfo = pGameConfig->servers[0];
    if(!GetServerManager()->Init(masterServerInfo.lanIP, masterServerInfo.tcpPort)) {
        LOGGER_LOG_ERROR("Failed to initialize netsocket on %s:%d", masterServerInfo.lanIP.c_str(), masterServerInfo.tcpPort);
        return 0;
    }
    LOGGER_LOG_INFO("Started netsocket on %s:%d", masterServerInfo.lanIP.c_str(), masterServerInfo.tcpPort);

    DatabaseConnectConfig dbConfig;
    dbConfig.host = pGameConfig->database.host.c_str();
    dbConfig.user = pGameConfig->database.user.c_str();
    dbConfig.pass = pGameConfig->database.pass.c_str();
    dbConfig.database = pGameConfig->database.database.c_str();
    dbConfig.port = pGameConfig->database.port;

    if(!GetContext()->GetDatabasePool()->Init(1, dbConfig)) {
        LOGGER_LOG_ERROR("Failed to initialize database pool");
        return 0;
    }
    LOGGER_LOG_INFO("Loaded %d workers for database", GetContext()->GetDatabasePool()->GetWorkerSize());

    if(!GetGameServer()->Init(masterServerInfo.wanIP, masterServerInfo.udpPort)) {
        LOGGER_LOG_ERROR("Failed to initialize game server on %s:%d", masterServerInfo.wanIP.c_str(), masterServerInfo.udpPort);
        return 0;
    }
    GetGameServer()->SetENetIncomeCmdType(pGameConfig->enetIncomeCmdType);
    LOGGER_LOG_INFO("Started game server on %s:%d", masterServerInfo.wanIP.c_str(), masterServerInfo.udpPort);

    //RegisterBalancedWorlds();

    if(pGameConfig->enableTelnetServer)
    {
        TelnetServer* pTelnetServer = GetTelnetServer();
        if(pTelnetServer->LoadTelnetConfigFromFile(GetProgramPath() + "/telnet_config.txt")) {
            if(!pTelnetServer->Init()) {
                LOGGER_LOG_ERROR("Failed to initialize telnet server on %s:%d", pTelnetServer->GetHost().c_str(), pTelnetServer->GetPort());
                return 0;
            }
            else {
                LOGGER_LOG_INFO("Started telnet server on %s:%d", pTelnetServer->GetHost().c_str(), pTelnetServer->GetPort());
            }
        }
        else {
            LOGGER_LOG_ERROR("Failed to load telnet_config.txt not gonna initialize telnet server");
        }
    }
    else
    {
        LOGGER_LOG_INFO("Not starting telnet server its disabled in config");
    }

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

    LOGGER_LOG_INFO("Killing Master server");

    GetLog()->Flush();

    if(dbThread.joinable()) dbThread.join();
    if(eventThread.joinable()) eventThread.join();

    GetTelnetServer()->Kill();
    GetGameServer()->Kill();
    GetServerManager()->Kill();

    GetLog()->Kill();
    GetContext()->Kill();

    mysql_library_end();
    return 0;
}
