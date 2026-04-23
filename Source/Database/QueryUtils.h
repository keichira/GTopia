#pragma once

#include "../Precompiled.h"
#include "../Utils/Variant.h"
#include "DatabaseResult.h"

enum eQueryStatus
{
    QUERY_STATUS_OK,
    QUERY_STATUS_TIMEOUT,
    QUERY_STATUS_FAIL
};

enum eQueryFlags
{
    QUERY_FLAG_NONE = 0 << 0,
    QUERY_FLAG_RETURN_RESULT = 1 << 0,
    QUERY_FLAG_PREPARED = 1 << 1,
    QUERY_FLAG_RETURN_INCREMENT = 1 << 2,
    QUERY_FLAG_BULK = 1 << 3
};

struct QueryTaskResult;

struct QueryRequest 
{
    QueryRequest(uint32 _ownerID) : ownerID(_ownerID) {}
    QueryRequest() {}

    uint32 ownerID = 0;
    VariantVector data; 
    VariantVector extraData;
    int32 queryID = -1;

    void (*callback)(QueryTaskResult&& result) = nullptr;

    template<typename... Args>
    QueryRequest& AddData(Args&&... args)
    {
        (data.emplace_back(std::forward<Args>(args)), ...);
        return *this;
    }

    template<typename... Args>
    QueryRequest& AddExtraData(Args&&... args)
    {
        (extraData.emplace_back(std::forward<Args>(args)), ...);
        return *this;
    }
};

struct QueryTaskRequest
{
    int32 ownerID = 0;
    VariantVector data;
    VariantVector extraData;
    string query = "";
    uint16 flags = 0;
    uint64 reqTime = 0;
    int32 queryID = -1;

    void (*callback)(QueryTaskResult&& result) = nullptr;
};

struct QueryTaskResult
{
    int32 ownerID = 0;
    VariantVector extraData;
    eQueryStatus status;
    uint64 increment = 0;
    DatabaseResult* result = nullptr;
    int32 queryID = -1;

    void (*callback)(QueryTaskResult&& result) = nullptr;

    Variant* GetExtraData(uint8 index)
    {
        if(index >= extraData.size()) {
            return nullptr;
        }

        return &extraData[index];
    }

    void Destroy()
    {
        SAFE_DELETE(result);
    }
};

struct TableQuery
{
    const char* query;
    uint16 flags = 0;
};

QueryTaskRequest MakeErrorQueryTask();