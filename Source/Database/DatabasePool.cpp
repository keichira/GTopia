#include "DatabasePool.h"
#include "DatabaseWorker.h"
#include "../IO/Log.h"
#include "../Utils/Timer.h"

DatabasePool::DatabasePool()
{
}

DatabasePool::~DatabasePool()
{
    Kill();
}

bool DatabasePool::Init(uint8 workerCount, const DatabaseConnectConfig& config)
{
    m_workers.reserve(workerCount);

    for(auto i = 0; i < workerCount; ++i) {
        DatabaseWorker* pWorker = new DatabaseWorker();

        if(!pWorker->Init(this, config)) {
            LOGGER_LOG_ERROR("Failed to start database worker %d/%d", (m_workers.size() + 1), workerCount);

            Kill();
            return false;
        }

        m_workers.push_back(pWorker);
    }

    return true;
}

void DatabasePool::Kill()
{
    for(auto& pWorker : m_workers) {
        SAFE_DELETE(pWorker);
    }

    m_workers.clear();
}

DatabaseWorker* DatabasePool::GetWorker(uint8 index)
{
    if(index >= m_workers.size()) {
        return nullptr;
    }

    return m_workers[index];
}

void DatabasePool::AddTask(QueryTaskRequest &&taskReq)
{
    taskReq.reqTime = Time::GetSystemTime();

    m_workers[taskReq.ownerID % m_workers.size()]->AddTask(std::move(taskReq));
}

bool DatabasePool::GetResult(QueryTaskResult& taskRes)
{
    return m_taskQueue.try_dequeue(taskRes);
}

void DatabasePool::AddResult(QueryTaskResult&& taskRes)
{
    m_taskQueue.enqueue(std::move(taskRes));
}

void DatabaseExec(DatabasePool* pPool, const string& query, QueryRequest& req, uint16 flags)
{
    if(!pPool) {
        return;
    }

    QueryTaskRequest taskReq;
    taskReq.query = query;
    taskReq.flags = flags;
    taskReq.data = req.data;
    taskReq.extraData = req.extraData;
    taskReq.ownerID = req.ownerID;

    pPool->AddTask(std::move(taskReq));
}