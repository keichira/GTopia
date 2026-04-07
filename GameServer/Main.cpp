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

bool firstCallShutdown = false;

#include <signal.h>
void SignalStop(int32 signum) 
{
    GetContext()->Shutdown();
    //GetContext()->Stop();
}

void ForceSaveEverything()
{
    GetMasterBroadway()->SendServerKillPacket();

    GetGameServer()->ForceSaveAllPlayers();
    GetWorldManager()->ForceSaveAllWorlds();

    QueryRequest req;
    req.ownerID = NET_ID_CONTEXT;

    req.extraData.resize(1);
    req.extraData[0] = (int32)-1;

    DatabaseExec(GetContext()->GetDatabasePool(), "", req, QUERY_FLAG_NONE); //end marker 
    firstCallShutdown = false;
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
    Timer lastLogWriteTime;
    uint64 nextTick = Time::GetSystemTime();

    while(GetContext()->IsRunning()) {
        DatabaseWorker* pWorker = GetContext()->GetDatabasePool()->GetWorker(0);
        pWorker->Update(30);

        if(lastLogWriteTime.GetElapsedTime() >= 1500) {
            GetLog()->Write();
            lastLogWriteTime.Reset();
        }

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
}

void EventThreadFunc() {
    while(GetContext()->IsRunning()) {
        if(!GetContext()->IsShutting()) {
            GetGameServer()->Update();
        }
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
                GetWorldManager()->OnHandleDatabase(std::move(taskRes));
                break;
            }

            /*case NET_ID_GAME_SERVER: {
                GetGameServer()->OnHandleDatabase(std::move(taskRes));
                break;
            }*/

            case NET_ID_CONTEXT: {
                if(GetContext()->IsShutting()) {
                    if(!taskRes.extraData.empty() && taskRes.extraData[0].GetINT() == -1) {
                        GetContext()->Stop();
                    }
                }

                break;
            }

            default: {       
                Player* pPlayer = GetGameServer()->GetPlayerByNetID(taskRes.ownerID);
                if(!pPlayer) {
                    LOGGER_LOG_WARN("Trying to process database result but player %d not exits? might be logged off", taskRes.ownerID);
                    break;
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

    if(!pItemMgr->LoadWikiData(GetProgramPath() + "/wiki_data.txt")) {
        LOGGER_LOG_ERROR("Failed to load wiki_data.txt!");
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

    std::unordered_map<string, uint32> hashData;
    for(auto& line : lines) {
        if(line.empty()) {
            continue;
        }

        auto args = Split(line, '|');
        hashData.insert_or_assign(args[0], ToUInt(args[1]));
    }

    pItemMgr->LoadFileHashes(hashData, false);
    pItemMgr->SaveToClientData(false);

    pItemMgr->LoadFileHashes(hashData, true);
    pItemMgr->SaveToClientData(true);

    /*PlayerTribute* pPlayerTrib = GetPlayerTributeManager();
    if(!pPlayerTrib->Load(GetProgramPath() + "/player_tribute.txt")) {
        LOGGER_LOG_ERROR("Failed to load player_tribute.txt anyways skipping it");
    }
    else {
        pPlayerTrib->SaveToClientData();
    }*/

    return true;
}

int main(int argc, char const* argv[])
{
    signal(SIGTERM, SignalStop);
    signal(SIGINT, SignalStop);
    signal(SIGSEGV, SignalStop);
    signal(SIGABRT, SignalStop);

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
    if(pGameConfig->LoadServersClient(GetProgramPath() + "/servers.txt", GetContext()->GetID()) != 2) {
        LOGGER_LOG_ERROR("Failed to load servers.txt");
        return 0;
    }

    if(pGameConfig->servers[1].serverType != CONFIG_SERVER_GAME) {
        LOGGER_LOG_ERROR("Woops trying to run server with wrong type %d it should be game", pGameConfig->servers[1].serverType);
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

    while(!GetContext()->IsShutting()) {
        if(GetMasterBroadway()->Connect(masterServerInfo.lanIP, masterServerInfo.tcpPort, 5, GetContext()->GetShutdownFlag())) {
            break;
        }
        else {
            LOGGER_LOG_ERROR("Failed to connect to master... killing");
            GetMasterBroadway()->Kill();
            GetContext()->Kill();
            GetLog()->Flush();
            GetLog()->Kill();
            return 0;
        }
    }

    LOGGER_LOG_INFO("Connected to master server");

    if(!GetRoleManager()->Load(GetProgramPath() + "/roles.txt")) {
        LOGGER_LOG_ERROR("Failed to load roles.txt");
        return 0;
    }

    if(!LoadItemData()) {
        return 0;
    }

    if(!GetPlayModManager()->Load(GetProgramPath() + "/playmods.txt")) {
        LOGGER_LOG_ERROR("Failed to load playmods.txt");
        //return 0;
    }

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
        LOGGER_LOG_ERROR("Failed to initialize game server on %s:%d", gameServerInfo.wanIP.c_str(), gameServerInfo.udpPort);
        return 0;
    }
    LOGGER_LOG_INFO("Started game server on %s:%d", gameServerInfo.wanIP.c_str(), gameServerInfo.udpPort);

    std::thread dbThread(DatabaseThreadFunc);
    std::thread eventThread(EventThreadFunc);
    
    uint64 nextTick = Time::GetSystemTime();

    while(GetContext()->IsRunning()) {
        uint64 tickStart = Time::GetSystemTime();
        
        if(!GetContext()->IsShutting()) {
            GetGameServer()->UpdateGameLogic(15);
        }

        GetMasterBroadway()->UpdateTCPLogic(15);
        ProcessDatabaseResults(15);

        if(GetContext()->IsShutting() && !firstCallShutdown) {
            ForceSaveEverything();
        }

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
    GetContext()->Kill();

    GetLog()->Flush();
    GetLog()->Kill();

    mysql_library_end();
    return 0;
}
