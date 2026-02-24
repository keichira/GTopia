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

const int32 TICK_RATE = 20;
const uint64 TICK_INTERVAL = 1000/TICK_RATE;

#include <signal.h>
void SignalStop(int32 signum) 
{ 
    LOGGER_LOG_WARN("Received signal %d", signum);
    GetContext()->Stop();
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

void DatabaseThreadFunc() {
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

        nextTick += (TICK_INTERVAL * 0.8);
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

void EventThreadFunc() {
    while(GetContext()->IsRunning()) {
        GetGameServer()->Update();
        GetMasterBroadway()->Update(true);

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


bool LoadItemData()
{
    ItemInfoManager* pItemMgr = GetItemInfoManager();

    if(!pItemMgr->Load(GetProgramPath() + "/items.txt")) {
        LOGGER_LOG_ERROR("Failed to load items.txt");
        return false;
    }

    File fileHashes;
    if(!fileHashes.Open(GetProgramPath() + "/filehashes.txt")) {
        LOGGER_LOG_ERROR("Failed to load filehashes.txt");
        return false;
    }

    uint32 fileSize = fileHashes.GetSize();
    string fileData(fileSize, '\0');

    if(fileHashes.Read(fileData.data(), fileSize) != fileSize) {
        return false;
    }
    auto lines = Split(fileData, '\n');

    std::vector<string> hashData;
    for(auto& line : lines) {
        if(line.empty()) {
            continue;
        }

        auto args = Split(line, '|');
        hashData.push_back(args[0]);
        hashData.push_back(args[1]);
    }

    pItemMgr->LoadFileHashes(hashData, false);
    pItemMgr->LoadItemsClientData(false);

    pItemMgr->LoadFileHashes(hashData, true);
    pItemMgr->LoadItemsClientData(true);
    return true;
}

int main(int argc, char const* argv[])
{
    signal(SIGTERM, SignalStop);
    signal(SIGINT, SignalStop);

    if(!ReadArgs(argc, argv)) {
        return 0;
    }

    LOGGER_LOG_INFO("Starting Game Server %d", GetContext()->GetID());

    GetContext()->Init();
    
    SetRandomSeed(Time::GetSystemTime());
    RandomizeRandomSeed();

    if(!GetLog()->InitFile(GetProgramPath() + "/logs/log_SERVER_" + ToString(GetContext()->GetID()) + ".txt")) {
        LOGGER_LOG_ERROR("Failed to init log file");
        return 0;
    }

    auto pGameConfig = GetContext()->GetGameConfig();
    if(pGameConfig->LoadServers(GetProgramPath() + "/servers.txt", GetContext()->GetID()) != 2) {
        LOGGER_LOG_ERROR("Failed to load servers.txt");
        return 0;
    }

    if(!pGameConfig->LoadConfig(GetProgramPath() + "/config.txt")) {
        LOGGER_LOG_ERROR("Failed to load config.txt");
        return 0;
    }

    auto gameServerInfo = pGameConfig->servers[1];
    if(!GetMasterBroadway()->Init(gameServerInfo.lanIP, gameServerInfo.tcpPort, 0)) {
        LOGGER_LOG_ERROR("Failed to initialize netsocket on %s:%d", gameServerInfo.lanIP.c_str(), gameServerInfo.tcpPort);
        return 0;
    }
    LOGGER_LOG_INFO("Started netsocket on %s:%d", gameServerInfo.lanIP.c_str(), gameServerInfo.tcpPort);

    LOGGER_LOG_INFO("Connecting to master server");
    auto masterServerInfo = pGameConfig->servers[0];
    GetMasterBroadway()->Connect(masterServerInfo.lanIP, masterServerInfo.tcpPort);

    uint64 connStartTime = Time::GetSystemTime();
    uint8 retryCount = 0;

    while(!GetMasterBroadway()->IsConnected() && GetContext()->IsRunning()) {
        GetMasterBroadway()->Update(true);

        if(retryCount == 5) {
            break;
        }

        uint64 now = Time::GetSystemTime();
        if(now - connStartTime >= 5000) {
            LOGGER_LOG_INFO("%d ms elapsed but still not connected to master... retrying (%d/5)", now - connStartTime, retryCount + 1);
            GetMasterBroadway()->RefreshForConnect();
            GetMasterBroadway()->Connect(masterServerInfo.lanIP, masterServerInfo.tcpPort);
            retryCount++;
            connStartTime = now;
        }

        SleepMS(10);
    }
    
    if(!GetMasterBroadway()->IsConnected()) {
        LOGGER_LOG_ERROR("Failed to connect to master... killing");
        GetMasterBroadway()->Kill();
        GetLog()->Flush();
        GetLog()->Kill();
        GetContext()->Kill();
        return 0;
    }

    if(!LoadItemData()) {
        return 0;
    }

    LOGGER_LOG_INFO("Connected to master server");
    GetMasterBroadway()->SendHelloPacket();

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

    if(!GetGameServer()->Init(gameServerInfo.wanIP, gameServerInfo.udpPort)) {
        LOGGER_LOG_ERROR("Failed to initialize game server on %s:%d", gameServerInfo.wanIP, gameServerInfo.udpPort);
        return 0;
    }
    LOGGER_LOG_INFO("Started game server on %s:%d", gameServerInfo.wanIP.c_str(), gameServerInfo.udpPort);

    std::thread dbThread(DatabaseThreadFunc);
    std::thread eventThread(EventThreadFunc);
    
    uint64 nextTick = Time::GetSystemTime();

    while(GetContext()->IsRunning()) {
        uint64 tickStart = Time::GetSystemTime();
        
        GetGameServer()->UpdateGameLogic(15);
        GetMasterBroadway()->UpdateTCPLogic(15);
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

    LOGGER_LOG_ERROR("Killing Game server %d", GetContext()->GetID());

    if(dbThread.joinable()) dbThread.join();
    if(eventThread.joinable()) eventThread.join();

    GetGameServer()->Kill();
    GetMasterBroadway()->Kill();

    GetLog()->Flush();
    GetLog()->Kill();
    GetContext()->Kill();

    mysql_library_end();
    return 0;
}
