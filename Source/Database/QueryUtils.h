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

struct QueryRequest
{
    int32 ownerID = 0;
    VariantVector data;
    VariantVector extraData;
};

struct QueryTaskRequest
{
    int32 ownerID = 0;
    VariantVector data;
    VariantVector extraData;
    string query = "";
    uint16 flags = 0;
    uint64 reqTime = 0;
};

struct QueryTaskResult
{
    int32 ownerID = 0;
    VariantVector extraData;
    eQueryStatus status;
    uint64 increment = 0;
    DatabaseResult* result = nullptr;

    void Destroy()
    {
        SAFE_DELETE(result);
    }
};

QueryTaskRequest MakeErrorQueryTask();