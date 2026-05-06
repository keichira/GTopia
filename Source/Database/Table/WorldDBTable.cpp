#include "WorldDBTable.h"

QueryRequest WorldDB::ExistsByName(const string& worldName, uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.AddData(worldName);

    req.queryID = DB_WORLD_EXISTS_BY_NAME;
    return req;
}

QueryRequest WorldDB::Create(const string& worldName, uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.AddData(worldName);

    req.queryID = DB_WORLD_CREATE;
    return req;
}

QueryRequest WorldDB::GetByName(const string& worldName, uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.AddData(worldName);

    req.queryID = DB_WORLD_GET_BY_NAME;
    return req;
}

QueryRequest WorldDB::Save(const string& worldName, uint32 worldID, uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.AddData(worldName, worldID);

    req.queryID = DB_WORLD_SAVE;
    return req;
}

QueryRequest WorldDB::GetByID(uint32 worldID, uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.AddData(worldID);

    req.queryID = DB_WORLD_GET_BY_ID;
    return req;
}

void DatabaseWorldExec(DatabasePool* pPool, QueryRequest& req, bool preapred)
{
    if(req.queryID < 0) {
        DatabaseExec(pPool, "", req, QUERY_FLAG_RETURN_RESULT); // fail query
        return;
    }

    TableQuery& query = sWorldQueryTable[req.queryID];

    if(preapred) {
        query.flags |= QUERY_FLAG_PREPARED;
    }

    DatabaseExec(
        pPool,
        query.query,
        req,
        query.flags
    );
}