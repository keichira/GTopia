#pragma once

#include "Precompiled.h"
#include "Utils/Timer.h"
#include "WorldRenderer.h"
#include <concurrentqueue.h>

struct WorldRenderInfo
{
    uint64 reqTime = 0;
    uint32 worldID = 0;
    uint32 userID = 0;
};

class WorldRendererManager {
public:
    WorldRendererManager();
    ~WorldRendererManager();

public:
    static WorldRendererManager* GetInstance()
    {
        static WorldRendererManager instance;
        return &instance;
    }

    void Update();
    void AddTask(uint32 userID, uint32 worldID);

private:
    moodycamel::ConcurrentQueue<WorldRenderInfo> m_renderQueue;
    Timer m_lastRenderTime;
    
    WorldRenderer* m_pRenderer; 
};

WorldRendererManager* GetWorldRendererManager();