#pragma once

#include "../Precompiled.h"
#include <concurrentqueue.h>
#include "DatabaseManager.h"
#include "QueryUtils.h"
#include "DatabaseWorker.h"

class DatabasePool {

    friend DatabaseWorker;

public:
    DatabasePool();
    ~DatabasePool();

public:
    bool Init(uint8 workerCount, const DatabaseConnectConfig& config);
    void Kill();

    uint32 GetWorkerSize() { return m_workers.size(); }
    DatabaseWorker* GetWorker(uint8 index);

    void AddTask(QueryTaskRequest&& taskReq);
    bool GetResult(QueryTaskResult& taskRes);

private:
    void AddResult(QueryTaskResult&& taskRes);

private:
    std::vector<DatabaseWorker*> m_workers;
    moodycamel::ConcurrentQueue<QueryTaskResult> m_taskQueue;
};

void DatabaseExec(DatabasePool* pPool, const string& query, QueryRequest& req, uint16 flags);