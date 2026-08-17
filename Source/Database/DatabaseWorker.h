#pragma once

#include "../Utils/Timer.h"
#include "DatabaseManager.h"
#include "PreparedParam.h"
#include "QueryUtils.h"
#include <concurrentqueue.h>

#define QUERY_TIMEOUT_MS 3000
#define PREPARED_PARAM_MAX_SIZE 15

class DatabasePool;

class DatabaseWorker
{
public:
    DatabaseWorker();
    ~DatabaseWorker();

public:
    bool Init(DatabasePool* pDbPool, const DatabaseConnectConfig& config);
    void Kill();

    void Update();
    void AddTask(QueryTaskRequest&& taskReq);

    bool IsConnected() { return (m_pDatabaseMgr && m_pDatabaseMgr->IsConnected()); }
    uint32 GetQueueSize() const { return m_taskQueue.size_approx(); }

private:
    bool SetupPreparedParams(VariantVector& params, bool bulk, uint32 startPos = 0);
    string EscapeStringRawParams(const Variant& var);
    bool BuildRawQueryParams(string& query, const VariantVector& params);
    void MakeFailedTaskAndAdd(QueryTaskRequest& taskReq, QueryTaskResult&& taskRes, eQueryStatus status);

private:
    DatabaseManager* m_pDatabaseMgr;
    PreparedParam* m_pPrepParam;
    DatabasePool* m_pDbPool;
    moodycamel::ConcurrentQueue<QueryTaskRequest> m_taskQueue;

    Timer m_lastConnTime;
    DatabaseConnectConfig m_config;
};