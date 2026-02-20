#pragma once

#include "Precompiled.h"
#include "DatabaseResult.h"
#include <mysql.h>

struct DatabaseConnectConfig
{
    const char* host = nullptr;
    const char* user = nullptr;
    const char* pass = nullptr;
    const char* database = nullptr;
    uint16 port = 3306;
    const char* unixSocket = nullptr;
    uint32 clientFlag = 0;
};

class DatabaseManager {
public:
    DatabaseManager();
    ~DatabaseManager();

public:
    bool Init(const DatabaseConnectConfig& config);
    void Kill();

    bool Query(const string& query);
    bool Query(const string& query, MYSQL_BIND* pBind);

    bool PrepareBulkStmt(const string& query);
    bool QueryBulk(MYSQL_BIND* pBind); /** if Query() called there would be a problem */

    uint64 GetLastInsertID();

    string EscapeString(const string& value);
    void PrintError();
    DatabaseResult* GetResults();

private:
    MYSQL_STMT* m_pLastStmt;
    MYSQL* m_pConnection;
};