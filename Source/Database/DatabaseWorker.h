#pragma once

#include "DatabaseManager.h"
#include <concurrentqueue.h>
#include "PreparedParam.h"
#include "QueryUtils.h"

#define QUERY_TIMEOUT_MS 3000
#define PREPARED_PARAM_MAX_SIZE 8

class DatabasePool;

class DatabaseWorker {
public:
    DatabaseWorker();
    ~DatabaseWorker();

public:
    bool Init(DatabasePool* pDbPool, const DatabaseConnectConfig& config);
    void Kill();

    void Update();

    void AddTask(QueryTaskRequest&& taskReq);

private:
    void SetupPreparedParams(VariantVector& params, bool bulk, uint32 startPos = 0);
    string EscapeStringRawParams(const Variant& var);
    bool BuildRawQueryParams(string& query, const VariantVector& params);
    void MakeFailedTaskAndAdd(QueryTaskRequest& taskReq, QueryTaskResult&& taskRes, eQueryStatus status);

private:
    DatabaseManager* m_pDatabaseMgr;
    PreparedParam* m_pPrepParam;
    DatabasePool* m_pDbPool;
    moodycamel::ConcurrentQueue<QueryTaskRequest> m_taskQueue;
};