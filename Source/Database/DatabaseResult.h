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

    int32 GetColumnIndex(const string& columnName) const 
    {
        auto it = m_columnIndices.find(columnName);
        return (it != m_columnIndices.end()) ? it->second : -1;
    }

    Variant& GetField(const string& fieldName, uint32 index) 
    {
        int32 colIndex = GetColumnIndex(fieldName);
        if(colIndex == -1 || index >= m_rows.size()) 
        {
            static Variant emptyVar;
            return emptyVar;
        }
        return m_rows[index][colIndex];
    }
    
    Variant* GetFieldSafe(const string& fieldName, uint32 index) 
    {
        if(index >= m_rows.size()) 
            return nullptr;
    
        int32 colIndex = GetColumnIndex(fieldName);
        if(colIndex == -1) 
            return nullptr;
    
        return &m_rows[index][colIndex];
    }

    Variant& GetValue(uint32 rowIdx, uint32 colIdx) { return m_rows[rowIdx][colIdx]; }
    
    VariantVector& GetRow(uint32 index) { return m_rows[index]; }
    uint32 GetRowCount() const { return m_rows.size(); }

private:
    void ParseNormal(MYSQL_RES* pResult, MYSQL_ROW& row, MYSQL_FIELD* fields, uint32 numFields);
    void ParseSTMT(MYSQL_STMT* pStmt, MYSQL_RES* pResult, MYSQL_ROW& row, MYSQL_FIELD* fields, uint32 numFields);
    Variant ExtractFieldVariant(MYSQL_FIELD& field, void* data, unsigned long length, bool isNull);

private:
    std::vector<string> m_columnNames;
    std::unordered_map<string, uint32> m_columnIndices;
    std::vector<VariantVector> m_rows;
};
