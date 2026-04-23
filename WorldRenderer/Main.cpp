#include "WorldRenderer.h"
#include "Context.h"
#include "Utils/ResourceManager.h"
#include "Item/ItemInfoManager.h"
#include "Utils/Timer.h"
#include "IO/Log.h"
#include "Math/Random.h"
#include "Utils/StringUtils.h"
#include "MasterBroadway.h"
#include "WorldRendererManager.h"

/**
 * 
 * RE DO IT :(
 * also it has high mem usage than other servers lol
 * 
 */

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

void EventThreadFunc() {
    while(GetContext()->IsRunning()) {
        GetMasterBroadway()->Update(true);

        SleepMS(10);
    }
}

#include "Utils/ZLibUtils.h"
#include "IO/File.h"

int main(int argc, char const* argv[])
{
    if(!ReadArgs(argc, argv)) {
        return 0;
    }

    LOGGER_LOG_INFO("Starting renderer server %d | %s", GetContext()->GetID(), Time::GetDateTimeStr().c_str());
    GetContext()->Init();

    SetRandomSeed(Time::GetSystemTime());
    RandomizeRandomSeed();

    GameConfig* pGameConfig = GetContext()->GetGameConfig();
    if(pGameConfig->LoadServersClient(GetProgramPath() + "/servers.txt", GetContext()->GetID()) != 2) {
        LOGGER_LOG_ERROR("Failed to load servers.txt");
        return 0;
    }

    if(pGameConfig->servers[1].serverType != CONFIG_SERVER_RENDERER) {
        LOGGER_LOG_ERROR("Woops trying to run server with wrong type %d it should be renderer", pGameConfig->servers[1].serverType);
        return 0;
    }

    if(!pGameConfig->LoadConfig(GetProgramPath() + "/config.txt")) {
        LOGGER_LOG_ERROR("Failed to load config.txt");
        return 0;
    }

    if(!IsFolderExists(pGameConfig->rendererStaticPath)) {
        LOGGER_LOG_ERROR("Static file %s is not exist?", pGameConfig->rendererStaticPath.c_str());
        return 0;
    }

    ResourceManager* pResMgr = GetResourceManager();
    pResMgr->SetResourcePath(pGameConfig->rendererStaticPath);

    auto renderServerInfo = pGameConfig->servers[1];
    if(!GetMasterBroadway()->Init(renderServerInfo.lanIP, renderServerInfo.tcpPort, 0)) {
        LOGGER_LOG_ERROR("Failed to initialize netsocket on %s:%d", renderServerInfo.lanIP.c_str(), renderServerInfo.tcpPort);
        return 0;
    }
    LOGGER_LOG_INFO("Started netsocket on %s:%d", renderServerInfo.lanIP.c_str(), renderServerInfo.tcpPort);

    LOGGER_LOG_INFO("Connecting to master server");
    auto masterServerInfo = pGameConfig->servers[0];

    GetContext()->LoadPreResources();

    while(GetContext()->IsRunning()) {
        if(GetMasterBroadway()->Connect(masterServerInfo.lanIP, masterServerInfo.tcpPort, 5, GetContext()->GetStopFlag())) {
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

    if(!GetItemInfoManager()->Load(GetProgramPath() + "/items.txt")) {
        LOGGER_LOG_ERROR("Failed to load items.txt");
        return 0;
    }

    GetMasterBroadway()->SendHelloPacket();
    WorldRendererManager* pRenderMgr = GetWorldRendererManager();
    MasterBroadway* pMasterBroadway = GetMasterBroadway();

    std::thread eventThrad(EventThreadFunc);

    while(GetContext()->IsRunning()) {
        pMasterBroadway->UpdateTCPLogic(15);
        pRenderMgr->Update();
        SleepMS(50);
    }

    GetMasterBroadway()->SendServerKillPacket();

    LOGGER_LOG_WARN("Killing renderer server");
    if(eventThrad.joinable()) eventThrad.join();

    GetMasterBroadway()->Kill();
    GetResourceManager()->Kill();
    GetContext()->Kill();
    GetLog()->Flush();
    GetLog()->Kill();
    return 0;
}
