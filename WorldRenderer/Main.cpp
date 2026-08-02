#include "Context.h"
#include "IO/Log.h"
#include "Item/ItemInfoManager.h"
#include "MasterBroadway.h"
#include "Math/Random.h"
#include "Utils/ResourceManager.h"
#include "Utils/StringUtils.h"
#include "Utils/Timer.h"
#include "WorldRenderer.h"
#include "WorldRendererManager.h"

/**
 *
 * RE DO IT :(
 * also it has high mem usage than other servers lol
 *
 */

bool firstCallShutdown = false;

#include <signal.h>
void SignalStop(int32 signum)
{
    LOGGER_LOG_WARN("Received signal %d", signum);
    GetContext()->Shutdown();
}

bool ReadArgs(int argc, char const* argv[])
{
    bool idSet = false;

    for (int i = 1; i < argc; ++i)
    {
        if (string(argv[i]) == "--id")
        {
            uint16 id = ToUInt(argv[i + 1]);
            if (id <= 0)
            {
                LOGGER_LOG_ERROR("server id must bigger than 0");
                return false;
            }

            GetContext()->SetID(id);
            idSet = true;
        }
    }

    if (!idSet)
    {
        LOGGER_LOG_ERROR("No --id param detected it must set!");
        return false;
    }

    return true;
}

void EventThreadFunc()
{
    while (GetContext()->IsRunning())
    {
        GetMasterBroadway()->Update(true);

        SleepMS(5);
    }
}

int main(int argc, char const* argv[])
{
    signal(SIGTERM, SignalStop);
    signal(SIGINT, SignalStop);
    signal(SIGSEGV, SignalStop);
    signal(SIGABRT, SignalStop);

    if (!ReadArgs(argc, argv))
        return 0;

    if (!GetLog()->InitFile(GetProgramPath() + "/logs/log_SERVER_" + ToString(GetContext()->GetID()) + ".txt"))
    {
        LOGGER_LOG_ERROR_ASAP("Failed to init log file, maybe try to create 'logs' folder?");
        return 0;
    }

    LOGGER_LOG_INFO_ASAP("Starting renderer server %d", GetContext()->GetID());
    LOGGER_LOG_INFO_ASAP("Project created by keichira https://github.com/keichira/GTopia");
    GetContext()->Init();

    SetRandomSeed(Time::GetSystemTime());
    RandomizeRandomSeed();

    GameConfig* pGameConfig = GetContext()->GetGameConfig();
    if (pGameConfig->LoadServersClient(GetProgramPath() + "/servers.txt", GetContext()->GetID()) != 2)
    {
        LOGGER_LOG_ERROR_ASAP("Failed to load servers.txt");
        return 0;
    }

    if (pGameConfig->servers[1].serverType != CONFIG_SERVER_RENDERER)
    {
        LOGGER_LOG_ERROR_ASAP("Woops trying to run server with wrong type %d it should be renderer",
                              pGameConfig->servers[1].serverType);
        return 0;
    }

    if (!pGameConfig->LoadConfig(GetProgramPath() + "/config.txt"))
    {
        LOGGER_LOG_ERROR_ASAP("Failed to load config.txt");
        return 0;
    }

    if (!IsFolderExists(pGameConfig->rendererStaticPath))
    {
        LOGGER_LOG_ERROR("Static folder '%s' is not exist?", pGameConfig->rendererStaticPath.c_str());
        return 0;
    }

    ResourceManager* pResMgr = GetResourceManager();
    pResMgr->SetResourcePath(pGameConfig->rendererStaticPath);

    if (!GetItemInfoManager()->Load(GetProgramPath() + "/items.txt"))
    {
        LOGGER_LOG_ERROR_ASAP("Failed to load items.txt");
        return 0;
    }

    auto renderServerInfo = pGameConfig->servers[1];
    if (!GetMasterBroadway()->Init(renderServerInfo.lanIP, renderServerInfo.tcpPort, 0))
    {
        LOGGER_LOG_ERROR_ASAP("Failed to initialize netsocket on %s:%d", renderServerInfo.lanIP.c_str(),
                              renderServerInfo.tcpPort);
        return 0;
    }
    LOGGER_LOG_INFO("Started netsocket on %s:%d", renderServerInfo.lanIP.c_str(), renderServerInfo.tcpPort);

    auto masterServerInfo = pGameConfig->servers[0];
    LOGGER_LOG_INFO_ASAP("Connecting and Authenticating with Master Server...");

    if (!GetMasterBroadway()->ConnectAndAuth(masterServerInfo.lanIP, masterServerInfo.tcpPort, 3,
                                             GetContext()->GetShutdownFlag()))
    {
        LOGGER_LOG_ERROR_ASAP("Master Server connection or authentication failed. Aborting startup.");
        GetMasterBroadway()->Kill();
        return 0;
    }

    WorldRendererManager* pRenderMgr = GetWorldRendererManager();
    MasterBroadway* pMasterBroadway = GetMasterBroadway();
    Context* pContext = GetContext();

    std::thread eventThrad(EventThreadFunc);

    Timer lastLogWriteTime;

    while (pContext->IsRunning())
    {
        if (pContext->IsShutting() && !firstCallShutdown)
        {
            firstCallShutdown = true;
            pMasterBroadway->SendServerKillPacket();
            pContext->Stop();
        }

        pMasterBroadway->UpdateTCPLogic(15);
        pRenderMgr->Update();

        if (!pContext->IsShutting() && lastLogWriteTime.GetElapsedTime() > 5000)
        {
            GetLog()->Write();
            lastLogWriteTime.Reset();
        }

        SleepMS(10);
    }

    LOGGER_LOG_WARN("Killing renderer server %d", GetContext()->GetID());
    if (eventThrad.joinable())
        eventThrad.join();

    GetMasterBroadway()->Kill();
    GetResourceManager()->Kill();
    GetContext()->Kill();
    GetLog()->Flush();
    GetLog()->Kill();
    return 0;
}
