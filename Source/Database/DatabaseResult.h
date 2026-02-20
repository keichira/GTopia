#pragma once

#include "../Precompiled.h"
#include "../Utils/Variant.h"
#include <mysql.h>

class DatabaseResult {
public:
    DatabaseResult();

public:
    static uint32 GetSizeOfField(MYSQL_FIELD& field);

public:
    bool Parse(MYSQL* pConnection, MYSQL_STMT* pStmt);

    Variant& GetField(const string& fieldName, uint32 index) { return m_results[index][fieldName]; }
    VariantMap& GetRow(uint32 index) { return m_results[index]; }
    uint32 GetRowCount() const { return m_results.size(); }

private:
    void ParseNormal(MYSQL_RES* pResult, MYSQL_ROW& row, MYSQL_FIELD* fields, uint32 numFields);
    void ParseSTMT(MYSQL_STMT* pStmt, MYSQL_RES* pResult, MYSQL_ROW& row, MYSQL_FIELD* fields, uint32 numFields);
    void ParseAndInsertField(MYSQL_FIELD& field, void* data, unsigned long length, bool isNull, VariantMap& variantMap);

private:
    std::vector<VariantMap> m_results;
};
