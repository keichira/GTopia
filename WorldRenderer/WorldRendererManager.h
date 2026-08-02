#pragma once

#include "Precompiled.h"
#include "Utils/Timer.h"
#include "World/WorldInfo.h"
#include "WorldRenderer.h"
#include <concurrentqueue.h>
#include <vector>

struct PlayerRenderData
{
    uint32 userID = 0;
    uint32 skinColor = 0;
    std::vector<int16> clothes;
};

struct WorldRenderInfo
{
    uint64 reqTime = 0;
    uint32 worldID = 0;
    uint32 userID = 0;
};

struct RenderJob
{
    uint32 userID = 0;
    uint32 worldID = 0;
    WorldInfo* pWorld = nullptr;
    std::vector<PlayerRenderData> players;
};

class WorldRendererManager
{
public:
    WorldRendererManager();
    ~WorldRendererManager();

    static WorldRendererManager* GetInstance()
    {
        static WorldRendererManager instance;
        return &instance;
    }

    void Update();
    bool AddTask(uint32 userID, uint32 worldID);
    void PushReadyJob(const RenderJob& job);

private:
    moodycamel::ConcurrentQueue<WorldRenderInfo> m_renderQueue;
    moodycamel::ConcurrentQueue<RenderJob> m_readyToDrawQueue;

    Timer m_lastRenderTime;
    Timer m_inactivityTimer;

    WorldRenderer m_renderer;

    const usize MAX_QUEUE_SIZE = 40;
};

WorldRendererManager* GetWorldRendererManager();