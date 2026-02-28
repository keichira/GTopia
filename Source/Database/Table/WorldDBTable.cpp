#include "WorldDBTable.h"

void DatabaseWorldExistsByName(DatabasePool* pPool, QueryRequest& req, bool prepared)
{
    uint16 flags = QUERY_FLAG_RETURN_RESULT;
    if(prepared) {
        flags |= QUERY_FLAG_PREPARED;
    }

    DatabaseExec(
        pPool, 
        "SELECT ID FROM Worlds WHERE Name = ? LIMIT 1;",
        req,
        flags
    );
}

QueryRequest MakeWorldExistsByName(const string& worldName, int32 ownerID)
{
    QueryRequest req;
    req.ownerID = ownerID;    

    req.data.resize(1);
    req.data[0] = worldName;
    return req;
}

void DatabaseWorldCreate(DatabasePool* pPool, QueryRequest& req, bool prepared)
{
    uint16 flags = QUERY_FLAG_RETURN_INCREMENT;
    if(prepared) {
        flags |= QUERY_FLAG_PREPARED;
    }

    DatabaseExec(
        pPool, 
        "INSERT INTO Worlds (Name, CreationDate, LastSeenTime) VALUES (?, SYSDATE(), NOW());",
        req,
        flags
    );
}

QueryRequest MakeWorldCreate(const string& worldName, int32 ownerID)
{
    QueryRequest req;
    req.ownerID = ownerID;

    req.data.resize(1);
    req.data[0] = worldName;
    return req;
}

void DatabaseGetWorldData(DatabasePool* pPool, QueryRequest& req, bool prepared)
{
    uint16 flags = QUERY_FLAG_RETURN_RESULT;
    if(prepared) {
        flags |= QUERY_FLAG_PREPARED;
    }

    DatabaseExec(
        pPool, 
        "SELECT ID, Name, Flags FROM Worlds WHERE Name = ?;",
        req,
        flags
    );
}

QueryRequest MakeGetWorldData(const string& worldName, int32 ownerID)
{
    QueryRequest req;
    req.ownerID = ownerID;

    req.data.resize(1);
    req.data[0] = worldName;
    return req;
}
