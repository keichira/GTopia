#include "WorldRendererManager.h"
#include "Context.h"
#include "IO/Log.h"
#include "MasterBroadway.h"
#include "Utils/ResourceManager.h"

WorldRendererManager::WorldRendererManager() {}

WorldRendererManager::~WorldRendererManager() {}

bool WorldRendererManager::AddTask(uint32 userID, uint32 worldID)
{
    if (m_renderQueue.size_approx() > MAX_QUEUE_SIZE)
    {
        LOGGER_LOG_ERROR("Render queue overloaded (%d items)! Rejecting request for User: %d, World: %d",
                         m_renderQueue.size_approx(), userID, worldID);
        GetMasterBroadway()->SendWorldRenderResult(false, userID, worldID);
        return false;
    }

    WorldRenderInfo renderInfo;
    renderInfo.reqTime = Time::GetSystemTime();
    renderInfo.userID = userID;
    renderInfo.worldID = worldID;

    m_renderQueue.enqueue(renderInfo);
    m_inactivityTimer.Reset();
    return true;
}

void WorldRendererManager::PushReadyJob(const RenderJob& job)
{
    m_readyToDrawQueue.enqueue(job);
}

void WorldRendererManager::Update()
{
    if (m_lastRenderTime.GetElapsedTime() >= 250)
    {
        RenderJob readyJob;
        if (m_readyToDrawQueue.try_dequeue(readyJob))
        {
            m_lastRenderTime.Reset();

            if (readyJob.pWorld)
            {
                m_renderer.Draw(readyJob.pWorld);

                LOGGER_LOG_INFO("World %d took %dms to draw", readyJob.worldID,
                                readyJob.renderStartTimer.GetElapsedTime());

                GetMasterBroadway()->SendWorldRenderResult(true, readyJob.userID, readyJob.worldID);
                SAFE_DELETE(readyJob.pWorld);
            }
            else
            {
                GetMasterBroadway()->SendWorldRenderResult(false, readyJob.userID, readyJob.worldID);
            }
        }
    }

    WorldRenderInfo renderInfo;
    if (m_renderQueue.try_dequeue(renderInfo))
    {
        if (renderInfo.worldID == 0)
        {
            GetMasterBroadway()->SendWorldRenderResult(false, renderInfo.userID, renderInfo.worldID);
            return;
        }

        if (Time::GetSystemTime() - renderInfo.reqTime > 5000)
        {
            LOGGER_LOG_ERROR("Render request timed out for world %d, skipping", renderInfo.worldID);
            GetMasterBroadway()->SendWorldRenderResult(false, renderInfo.userID, renderInfo.worldID);
            return;
        }

        WorldInfo* pWorld = new WorldInfo();

        LOGGER_LOG_INFO("Loading world %d", renderInfo.worldID);
        if (!m_renderer.LoadWorld(renderInfo.worldID, pWorld))
        {
            LOGGER_LOG_ERROR("Failed to load world %d file, skipping", renderInfo.worldID);
            GetMasterBroadway()->SendWorldRenderResult(false, renderInfo.userID, renderInfo.worldID);
            SAFE_DELETE(pWorld);
            return;
        }

        RenderJob job;
        job.userID = renderInfo.userID;
        job.worldID = renderInfo.worldID;
        job.pWorld = pWorld;
        job.renderStartTimer.Reset();

        PushReadyJob(job);
    }
    else
    {
        if (m_inactivityTimer.GetElapsedTime() >= 180 * 60 * 1000)
        {
            LOGGER_LOG_INFO("System idle for 180 mins. Flushing resource manager cache...");
            GetResourceManager()->Kill();
            m_inactivityTimer.Reset();
        }
    }
}

WorldRendererManager* GetWorldRendererManager()
{
    return WorldRendererManager::GetInstance();
}