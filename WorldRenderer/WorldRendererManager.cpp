#include "WorldRendererManager.h"
#include "WorldRenderer.h"
#include "Utils/Timer.h"
#include "MasterBroadway.h"
#include "IO/Log.h"
#include "Utils/ResourceManager.h"

WorldRendererManager::WorldRendererManager()
{
}

WorldRendererManager::~WorldRendererManager()
{
}

void WorldRendererManager::Update()
{
    if(m_lastRenderTime.GetElapsedTime() <= 250) {
        return;
    }

    WorldRenderInfo renderInfo;
    if(!m_renderQueue.try_dequeue(renderInfo)) {
        if(m_lastRenderTime.GetElapsedTime() >= 2 * 3600 * 1000) {
            LOGGER_LOG_INFO("Killing resources due inactivity");
            GetResourceManager()->Kill();
            m_lastRenderTime.Reset();
        }

        return;
    }

    m_lastRenderTime.Reset();
    MasterBroadway* pMasterBroadway = GetMasterBroadway();

    if(renderInfo.worldID == 0) {
        LOGGER_LOG_ERROR("World id is 0? skippping");
        pMasterBroadway->SendWorldRenderResult(false, renderInfo.userID, renderInfo.worldID);
        return;
    }

    uint64 timeNow = Time::GetSystemTime();
    if(timeNow - renderInfo.reqTime > 5000) {
        LOGGER_LOG_ERROR("Render time exceed %d skipping", timeNow - renderInfo.reqTime);
        pMasterBroadway->SendWorldRenderResult(false, renderInfo.userID, renderInfo.worldID);
        return;
    }

    uint64 timeLoad = Time::GetSystemTime();
    WorldRenderer renderWorld;
    if(!renderWorld.LoadWorld(renderInfo.worldID)) {
        LOGGER_LOG_ERROR("Failed to load world %d, skipping", renderInfo.worldID);
        pMasterBroadway->SendWorldRenderResult(false, renderInfo.userID, renderInfo.worldID);
        return;   
    }
    uint64 timeLoadEnd = Time::GetSystemTime();

    uint64 timeDraw = Time::GetSystemTime();
    renderWorld.Draw();
    uint64 timeDrawEnd = Time::GetSystemTime(); 

    pMasterBroadway->SendWorldRenderResult(true, renderInfo.userID, renderInfo.worldID);

    uint64 finalLoadTime = timeLoadEnd - timeLoad;
    uint64 finalDRawTime = timeDrawEnd - timeDraw;
    LOGGER_LOG_INFO("Rendered world %d requested: %d loadTime: %dms, drawTime: %dms, total %dms", renderInfo.worldID, renderInfo.userID, finalLoadTime, finalDRawTime, finalLoadTime + finalDRawTime );
}

void WorldRendererManager::AddTask(uint32 userID, uint32 worldID)
{
    WorldRenderInfo renderInfo;
    renderInfo.reqTime = Time::GetSystemTime();
    renderInfo.userID = userID;
    renderInfo.worldID = worldID;

    m_renderQueue.enqueue(std::move(renderInfo));
}

WorldRendererManager* GetWorldRendererManager() { return WorldRendererManager::GetInstance(); }
