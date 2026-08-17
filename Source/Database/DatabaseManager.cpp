#include "DatabaseManager.h"
#include "IO/Log.h"

DatabaseManager::DatabaseManager() : m_pConnection(nullptr), m_pLastStmt(nullptr) {}

DatabaseManager::~DatabaseManager()
{
    Kill();
}

bool DatabaseManager::Init(const DatabaseConnectConfig& config)
{
    Kill();

    m_pConnection = mysql_init(nullptr);
    if (!m_pConnection)
    {
        PrintError();
        return false;
    }

    if (!mysql_real_connect(m_pConnection, config.host, config.user, config.pass, config.database, config.port,
                            config.unixSocket, config.clientFlag))
    {
        PrintError();
        Kill();
        return false;
    }

    return true;
}

void DatabaseManager::Kill()
{
    if (m_pConnection)
    {
        mysql_close(m_pConnection);
        m_pConnection = nullptr;
    }

    if (m_pLastStmt)
    {
        mysql_stmt_close(m_pLastStmt);
        m_pLastStmt = nullptr;
    }
}

bool DatabaseManager::Query(const string& query)
{
    if (!m_pConnection)
        return false;

    if (mysql_query(m_pConnection, query.c_str()))
    {
        PrintError();
        return false;
    }

    return true;
}

bool DatabaseManager::Query(const string& query, MYSQL_BIND* pBind)
{
    if (!m_pConnection)
        return false;

    if (m_pLastStmt)
    {
        mysql_stmt_close(m_pLastStmt);
        m_pLastStmt = nullptr;
    }

    MYSQL_STMT* pStmt = mysql_stmt_init(m_pConnection);
    if (!pStmt)
    {
        PrintError();
        return false;
    }

    if (mysql_stmt_prepare(pStmt, query.c_str(), query.length()) != 0)
    {
        PrintError();
        mysql_stmt_close(pStmt);
        m_pLastStmt = nullptr;
        return false;
    }

    if (mysql_stmt_bind_param(pStmt, pBind) != 0)
    {
        PrintError();
        mysql_stmt_close(pStmt);
        m_pLastStmt = nullptr;
        return false;
    }

    if (mysql_stmt_execute(pStmt) != 0)
    {
        PrintError();
        mysql_stmt_close(pStmt);
        m_pLastStmt = nullptr;
        return false;
    }

    m_pLastStmt = pStmt;
    // mysql_stmt_close(pStmt);
    return true;
}

bool DatabaseManager::PrepareBulkStmt(const string& query)
{
    if (!m_pConnection)
        return false;

    if (m_pLastStmt)
    {
        mysql_stmt_close(m_pLastStmt);
        m_pLastStmt = nullptr;
    }

    MYSQL_STMT* pStmt = mysql_stmt_init(m_pConnection);
    if (!pStmt)
    {
        PrintError();
        return false;
    }

    if (mysql_stmt_prepare(pStmt, query.c_str(), query.length()) != 0)
    {
        PrintError();
        mysql_stmt_close(pStmt);
        m_pLastStmt = nullptr;
        return false;
    }

    m_pLastStmt = pStmt;
    return true;
}

bool DatabaseManager::QueryBulk(MYSQL_BIND* pBind)
{
    if (!m_pConnection)
        return false;

    if (!m_pLastStmt)
        return false;

    if (mysql_stmt_bind_param(m_pLastStmt, pBind) != 0)
    {
        PrintError();
        return false;
    }

    if (mysql_stmt_execute(m_pLastStmt) != 0)
    {
        PrintError();
        return false;
    }

    mysql_stmt_reset(m_pLastStmt);
    return true;
}

bool DatabaseManager::BeginTransaction()
{
    if (!m_pConnection)
        return false;

    if (mysql_autocommit(m_pConnection, 0) != 0)
    {
        PrintError();
        return false;
    }
    return true;
}

bool DatabaseManager::Commit()
{
    if (!m_pConnection)
        return false;

    if (mysql_commit(m_pConnection) != 0)
    {
        PrintError();
        return false;
    }

    mysql_autocommit(m_pConnection, 1);
    return true;
}

bool DatabaseManager::Rollback()
{
    if (!m_pConnection)
        return false;

    bool result = (mysql_rollback(m_pConnection) == 0);
    if (!result)
    {
        PrintError();
    }

    mysql_autocommit(m_pConnection, 1);
    return result;
}

uint64 DatabaseManager::GetLastInsertID()
{
    return mysql_insert_id(m_pConnection);
}

string DatabaseManager::EscapeString(const string& value)
{
    if (!m_pConnection || value.empty())
        return "";

    string res;
    res.resize(value.size() * 2 + 1);

    unsigned long escapedLength = mysql_real_escape_string(m_pConnection, &res[0], value.data(), value.size());

    res.resize(escapedLength);
    return res;
}

void DatabaseManager::PrintError()
{
    if (!m_pConnection)
        return;

    int32 err = mysql_errno(m_pConnection);
    const char* errStr = mysql_error(m_pConnection);

    if (err == CR_SERVER_GONE_ERROR || err == CR_SERVER_LOST)
    {
        LOGGER_LOG_ERROR("CRITICAL: Database connection lost! Error: (%d) %s. Cleaning up connection...", err, errStr);
        Kill();
    }
    else
    {
        LOGGER_LOG_ERROR("MySQL Error: (%d) %s", err, errStr);
    }
}

DatabaseResult* DatabaseManager::GetResults()
{
    if (!m_pConnection)
        return nullptr;

    DatabaseResult* pResult = new DatabaseResult();
    if (!pResult->Parse(m_pConnection, m_pLastStmt))
    {
        SAFE_DELETE(pResult);
        return nullptr;
    }

    return pResult;
}
