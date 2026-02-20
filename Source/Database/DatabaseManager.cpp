#include "DatabaseManager.h"
#include "IO/Log.h"

DatabaseManager::DatabaseManager()
: m_pConnection(nullptr), m_pLastStmt(nullptr)
{
}

DatabaseManager::~DatabaseManager()
{
    Kill();
}

bool DatabaseManager::Init(const DatabaseConnectConfig& config)
{
    m_pConnection = mysql_init(nullptr);
    if(!m_pConnection) {
        PrintError();
        return false;
    }

    if(!mysql_real_connect(m_pConnection, config.host, config.user, config.pass, config.database, config.port, config.unixSocket, config.clientFlag)) {
        PrintError();
        Kill();
        return false;
    }

    return true;
}

void DatabaseManager::Kill()
{
    if(m_pConnection) {
        mysql_close(m_pConnection);
        m_pConnection = nullptr;
    }

    if(m_pLastStmt) {
        mysql_stmt_close(m_pLastStmt);
        m_pLastStmt = nullptr;
    }
}

bool DatabaseManager::Query(const string& query)
{
    if(!m_pConnection) {
        return false;
    }

    if(mysql_query(m_pConnection, query.c_str())) {
        PrintError();
        return false;
    }

    return true;
}

bool DatabaseManager::Query(const string& query, MYSQL_BIND* pBind)
{
    if(!m_pConnection) {
        return false;
    }

    if(m_pLastStmt) {
        mysql_stmt_close(m_pLastStmt);
        m_pLastStmt = nullptr;
    }

    MYSQL_STMT* pStmt = mysql_stmt_init(m_pConnection);
    if(!pStmt) {
        PrintError();
        return false;
    }

    if (mysql_stmt_prepare(pStmt, query.c_str(), query.length()) != 0) {
        PrintError();
        mysql_stmt_close(pStmt);
        m_pLastStmt = nullptr;
        return false;
    }

    if(mysql_stmt_bind_param(pStmt, pBind) != 0) {
        PrintError();
        mysql_stmt_close(pStmt);
        m_pLastStmt = nullptr;
        return false;
    }

    if(mysql_stmt_execute(pStmt) != 0) {
        PrintError();
        mysql_stmt_close(pStmt);
        m_pLastStmt = nullptr;
        return false;
    }

    m_pLastStmt = pStmt;
    //mysql_stmt_close(pStmt);
    return true;
}

bool DatabaseManager::PrepareBulkStmt(const string& query)
{
    if(!m_pConnection) {
        return false;
    }

    if(m_pLastStmt) {
        mysql_stmt_close(m_pLastStmt);
        m_pLastStmt = nullptr;
    }

    MYSQL_STMT* pStmt = mysql_stmt_init(m_pConnection);
    if(!pStmt) {
        PrintError();
        return false;
    }

    if(mysql_stmt_prepare(pStmt, query.c_str(), query.length()) != 0) {
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
    if(!m_pConnection) {
        return false;
    }

    if(!m_pLastStmt) {
        return false;
    }

    if(mysql_stmt_bind_param(m_pLastStmt, pBind) != 0) {
        PrintError();
        return false;
    }

    if (mysql_stmt_execute(m_pLastStmt) != 0) {
        PrintError();
        return false;
    }

    mysql_stmt_reset(m_pLastStmt);
    return true;
}

uint64 DatabaseManager::GetLastInsertID()
{
    return mysql_insert_id(m_pConnection);
}

string DatabaseManager::EscapeString(const string& value)
{
    char* buffer = new char[value.size() * 2 + 1];

    mysql_real_escape_string(m_pConnection, buffer, value.c_str(), value.size());

    string res = buffer;
    SAFE_DELETE_ARRAY(buffer);

    return res;
}

void DatabaseManager::PrintError()
{
    LOGGER_LOG_ERROR("MySQL Error: %s", mysql_error(m_pConnection));
}

DatabaseResult* DatabaseManager::GetResults()
{
    if(!m_pConnection) {
        return nullptr;
    }

    DatabaseResult* pResult = new DatabaseResult();
    if(!pResult->Parse(m_pConnection, m_pLastStmt)) {
        SAFE_DELETE(pResult);
        return nullptr;
    }

    return pResult;
}
