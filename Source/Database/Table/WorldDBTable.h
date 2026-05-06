#pragma once

#include "../DatabasePool.h"

static TableQuery sWorldQueryTable[] =
{
    {"SELECT ID FROM Worlds WHERE Name = ? LIMIT 1;", QUERY_FLAG_RETURN_RESULT},
    {"INSERT INTO Worlds (Name, CreationDate, LastSeenTime) VALUES (?, SYSDATE(), NOW());", QUERY_FLAG_RETURN_INCREMENT},
    {"SELECT ID, Name, Flags FROM Worlds WHERE Name = ? LIMIT 1;", QUERY_FLAG_RETURN_RESULT},
    {"UPDATE Worlds SET LastSeenTime = NOW(), Name = ? WHERE ID = ?;", QUERY_FLAG_NONE},
    {"SELECT ID, Name, Flags FROM Worlds WHERE ID = ?;", QUERY_FLAG_RETURN_RESULT},
};

enum eWorldDBQuery
{
    DB_WORLD_EXISTS_BY_NAME,
    DB_WORLD_CREATE,
    DB_WORLD_GET_BY_NAME,
    DB_WORLD_SAVE,
    DB_WORLD_GET_BY_ID
};

namespace WorldDB
{
    QueryRequest ExistsByName(const string& worldName, uint32 ownerID = 0);
    QueryRequest Create(const string& worldName, uint32 ownerID = 0);
    QueryRequest GetByName(const string& worldName, uint32 ownerID = 0);
    QueryRequest Save(const string& worldName, uint32 worldID, uint32 ownerID = 0);
    QueryRequest GetByID(uint32 worldID, uint32 ownerID = 0);
}

void DatabaseWorldExec(DatabasePool* pPool, QueryRequest& req, bool preapred = false);