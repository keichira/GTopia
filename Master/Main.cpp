#include "Context.h"
#include "IO/Log.h"
#include "Math/Random.h"
#include "Utils/Timer.h"
#include "Server/GameServer.h"
#include "Server/ServerManager.h"

const int32 TICK_RATE = 20;
const uint64 TICK_INTERVAL = 1000/TICK_RATE;

#include <signal.h>
void SignalStop(int32 signum) 
{ 
    LOGGER_LOG_WARN("Received signal %d", signum);
    GetContext()->Stop();
}

void DatabaseThreadFunc() 
{
    uint64 lastLogWriteTime = Time::GetSystemTime();
    uint64 nextTick = Time::GetSystemTime();

    while(GetContext()->IsRunning()) {
        DatabaseWorker* pWorker = GetContext()->GetDatabasePool()->GetWorker(0);
        pWorker->Update();

        uint64 logWriteStart = Time::GetSystemTime();
        if(logWriteStart - lastLogWriteTime >= 1000) {
            GetLog()->Write();
            lastLogWriteTime = Time::GetSystemTime();
        }

        nextTick += (TICK_INTERVAL * 0.6);
        uint64 checkTime = Time::GetSystemTime();
        if(checkTime > nextTick) {
            // lag
            nextTick = checkTime;
        }

        uint64 sleepTime = nextTick - Time::GetSystemTime();
        if(sleepTime > 0) {
            SleepMS(sleepTime);
        }
    }
}

void EventThreadFunc() 
{
    while(GetContext()->IsRunning()) {
        GetGameServer()->Update();
        GetServerManager()->Update(false);

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
    uint32 processed = 0;

    while(pDatabasePool->GetResult(taskRes)) {
        switch(taskRes.ownerID) {
            case NET_ID_FALLBACK: {
                break;
            }

            case NET_ID_WORLD_MANAGER: {
                break;
            }

            default: {       
                Player* pPlayer = GetGameServer()->GetPlayerByNetID(taskRes.ownerID);
                if(!pPlayer) {
                    LOGGER_LOG_WARN("Trying to process database result but player %d not exits?", taskRes.ownerID);
                }

                pPlayer->OnHandleDatabase(std::move(taskRes));
            }
        }

        processed++;
        if(Time::GetSystemTime() - startTime >= maxTimeMS) {
            break;
        }
    }

    if(processed > 0) {
        LOGGER_LOG_DEBUG("Processed %d Database result maxMS %d, took %d MS", processed, maxTimeMS, Time::GetSystemTime() - startTime);
    }
}

int main(int argc, char const* argv[])
{
    signal(SIGTERM, SignalStop);
    signal(SIGINT, SignalStop);

    LOGGER_LOG_INFO("Starting Master Server");
    
    GetContext()->Init();

    SetRandomSeed(Time::GetSystemTime());
    RandomizeRandomSeed();

    GetContext()->SetID(0);
    if(!GetLog()->InitFile(GetProgramPath() + "/logs/log_MASTER.txt")) {
        LOGGER_LOG_ERROR("Failed to init log file");
        return 0;
    }

    auto pGameConfig = GetContext()->GetGameConfig();
    if(pGameConfig->LoadServers(GetProgramPath() + "/servers.txt") == 0) {
        LOGGER_LOG_ERROR("Failed to load servers.txt");
        return 0;
    }
    LOGGER_LOG_INFO("Loaded %d servers from servers.txt", pGameConfig->servers.size());

    if(!pGameConfig->LoadConfig(GetProgramPath() + "/config.txt")) {
        LOGGER_LOG_ERROR("Failed to load config.txt");
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
    LOGGER_LOG_INFO("Started game server on %s:%d", masterServerInfo.wanIP.c_str(), masterServerInfo.udpPort);

    std::thread dbThread(DatabaseThreadFunc);
    std::thread eventThread(EventThreadFunc);
    
    uint64 nextTick = Time::GetSystemTime();

    while(GetContext()->IsRunning()) {
        uint64 tickStart = Time::GetSystemTime();
        
        GetGameServer()->UpdateGameLogic(15);
        GetServerManager()->UpdateTCPLogic(15);
        ProcessDatabaseResults(15);

        nextTick += TICK_INTERVAL;

        uint64 checkTime = Time::GetSystemTime();
        if(checkTime > nextTick) {
            // lag
            nextTick = checkTime;
        }

        uint64 sleepTime = nextTick - Time::GetSystemTime();
        if(sleepTime > 0) {
            SleepMS(sleepTime);
        }
    }

    LOGGER_LOG_INFO("Killing Master server");

    if(dbThread.joinable()) dbThread.join();
    if(eventThread.joinable()) eventThread.join();

    GetGameServer()->Kill();
    GetServerManager()->Kill();

    GetLog()->Flush();
    GetLog()->Kill();
    GetContext()->Kill();

    mysql_library_end();
    return 0;
}
