#include "DatabaseWorker.h"
#include "../Utils/Timer.h"
#include "DatabasePool.h"
#include "../Utils/StringUtils.h"

DatabaseWorker::DatabaseWorker()
: m_pDatabaseMgr(nullptr), m_pPrepParam(nullptr)
{
}

DatabaseWorker::~DatabaseWorker()
{
    Kill();
}


bool DatabaseWorker::Init(DatabasePool* pDbPool, const DatabaseConnectConfig& config)
{
    m_pDatabaseMgr = new DatabaseManager();
    if(!m_pDatabaseMgr->Init(config)) {
        Kill();
        return false;
    }

    m_pDbPool = pDbPool;
    m_pPrepParam = new PreparedParam(PREPARED_PARAM_MAX_SIZE);
    return true;
}

void DatabaseWorker::Kill()
{
    SAFE_DELETE(m_pDatabaseMgr);
    SAFE_DELETE(m_pPrepParam);
}

void DatabaseWorker::Update()
{
    if(!m_pDatabaseMgr) {
        return;
    }

    uint64 updateStartTime = Time::GetSystemTime();
    QueryTaskRequest taskReq;

    while(m_taskQueue.try_dequeue(taskReq)) {
        uint64 taskStartTime = Time::GetSystemTime();

        QueryTaskResult taskRes;
        if(taskReq.query.empty()) {
            MakeFailedTaskAndAdd(taskReq, std::move(taskRes), QUERY_STATUS_FAIL);
            continue;
        }

        if(taskStartTime - taskReq.reqTime >= QUERY_TIMEOUT_MS) {
            MakeFailedTaskAndAdd(taskReq, std::move(taskRes), QUERY_STATUS_TIMEOUT);
            continue;
        }

        bool succeed = false;
        if((taskReq.flags & QUERY_FLAG_PREPARED)) {
            if(taskReq.flags & QUERY_FLAG_BULK) { // actually i have no idea why im doing that
                uint32 querySize = (taskReq.data.size() - 1) / taskReq.data[0].GetUINT();
                if(!m_pDatabaseMgr->PrepareBulkStmt(taskReq.query)) {
                    // what should we do wtf
                }

                for(uint32 i = 0; i < querySize; ++i) {
                    SetupPreparedParams(taskReq.data, true, 1 + (i - 1)*(taskReq.data[0].GetUINT()));
                    m_pDatabaseMgr->QueryBulk(m_pPrepParam->GetBinds().data());
                }
                break;
            }
            else {
                SetupPreparedParams(taskReq.data, false);
                succeed = m_pDatabaseMgr->Query(taskReq.query, m_pPrepParam->GetBinds().data());
            }
            m_pPrepParam->Reset();
        }
        else {
            if(!BuildRawQueryParams(taskReq.query, taskReq.data)) {
                MakeFailedTaskAndAdd(taskReq, std::move(taskRes), QUERY_STATUS_FAIL);
                continue;
            }

            succeed = m_pDatabaseMgr->Query(taskReq.query);
        }

        if(succeed && (taskReq.flags & QUERY_FLAG_RETURN_RESULT)) {
            taskRes.result = m_pDatabaseMgr->GetResults();
        }

        if(succeed && (taskReq.flags & QUERY_FLAG_RETURN_INCREMENT)) {
            taskRes.increment = m_pDatabaseMgr->GetLastInsertID();
        }

        taskRes.ownerID = taskReq.ownerID;
        taskRes.extraData = std::move(taskReq.extraData);
        taskRes.status = succeed ? QUERY_STATUS_OK : QUERY_STATUS_FAIL;
        m_pDbPool->AddResult(std::move(taskRes));
    }
}

void DatabaseWorker::AddTask(QueryTaskRequest&& taskReq)
{
    m_taskQueue.enqueue(taskReq);
}

void DatabaseWorker::SetupPreparedParams(VariantVector& params, bool bulk, uint32 startPos)
{
    uint32 paramSize = bulk ? params[0].GetUINT() : params.size();

    for(auto i = startPos; i < (paramSize + startPos); ++i) {
        switch(params[i].GetType()) {
            case VARIANT_TYPE_INT: {
                m_pPrepParam->SetInt(i, params[i].GetINT()); break;
            }
            case VARIANT_TYPE_UINT: {
                m_pPrepParam->SetInt(i, params[i].GetUINT()); break;
            }
            case VARIANT_TYPE_STRING: {
                m_pPrepParam->SetString(i, params[i].GetString()); break;
            }
        }
    }
}

string DatabaseWorker::EscapeStringRawParams(const Variant& var)
{
    switch(var.GetType()) {
        case VARIANT_TYPE_INT:
            return  m_pDatabaseMgr->EscapeString(ToString(var.GetINT()));
        case VARIANT_TYPE_UINT:
            return m_pDatabaseMgr->EscapeString(ToString(var.GetUINT()));
        case VARIANT_TYPE_FLOAT:
            return m_pDatabaseMgr->EscapeString(ToString(var.GetFloat()));
        case VARIANT_TYPE_STRING:
            return "'" + m_pDatabaseMgr->EscapeString(var.GetString()) + "'";

        default:
            return "";
    }
}

bool DatabaseWorker::BuildRawQueryParams(string& query, const VariantVector& params)
{
    if(CountCharacter(query, '?') != params.size()) {
        return false;
    }

    uint32 lastPos = 0;
    uint32 paramIndex = 0;

    string res;

    for(uint32 i = 0; i < query.size(); ++i) {
        if(query[i] == '?') {
            res.append(query, lastPos, i - lastPos);

            string value = EscapeStringRawParams(params[paramIndex]);
            if(value.empty() && params[paramIndex].GetType() != VARIANT_TYPE_STRING) {
                return false;
            }

            res.append(value);
            lastPos = i + 1;
            paramIndex++;
        }
    }

    if (lastPos < query.size())
        res.append(query, lastPos, query.size() - lastPos);

    query = std::move(res);
    return true;
}

void DatabaseWorker::MakeFailedTaskAndAdd(QueryTaskRequest& taskReq, QueryTaskResult&& taskRes, eQueryStatus status)
{
    taskRes.extraData = std::move(taskReq.extraData);
    taskRes.status = status;
    taskRes.ownerID = taskReq.ownerID;

    m_pDbPool->AddResult(std::move(taskRes));
}