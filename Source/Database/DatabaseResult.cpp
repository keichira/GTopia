#include "DatabaseResult.h"
#include "../IO/Log.h"
#include "../Utils/StringUtils.h"
#include "PreparedParam.h"
#include <ctime>

DatabaseResult::DatabaseResult() {}

bool DatabaseResult::Parse(MYSQL* pConnection, MYSQL_STMT* pStmt)
{
    if (!pConnection)
        return false;

    MYSQL_RES* pResult = pStmt ? mysql_stmt_result_metadata(pStmt) : mysql_store_result(pConnection);
    if (!pResult)
        return false;

    MYSQL_ROW row;
    uint32 numFields = mysql_num_fields(pResult);

    MYSQL_FIELD* fields = mysql_fetch_fields(pResult);
    if (!fields)
    {
        if (!pStmt)
        {
            mysql_free_result(pResult);
        }

        return false;
    }

    if (pStmt)
    {
        ParseSTMT(pStmt, pResult, row, fields, numFields);
    }
    else
    {
        ParseNormal(pResult, row, fields, numFields);
    }

    mysql_free_result(pResult);
    return true;
}

uint32 DatabaseResult::GetSizeOfField(MYSQL_FIELD& field)
{
    switch (field.type)
    {
        case MYSQL_TYPE_BOOL:
        case MYSQL_TYPE_TINY:
            return sizeof(int8);
        case MYSQL_TYPE_SHORT:
            return sizeof(int16);
        case MYSQL_TYPE_LONG:
            return sizeof(int32);
        case MYSQL_TYPE_LONGLONG:
            return sizeof(int64);
        case MYSQL_TYPE_FLOAT:
            return sizeof(float);
        case MYSQL_TYPE_DOUBLE:
            return sizeof(double);

        case MYSQL_TYPE_VAR_STRING:
        case MYSQL_TYPE_STRING:
            return field.length + 1;

        default:
            return field.length > 0 ? field.length : 256;
    }
}

void DatabaseResult::ParseNormal(MYSQL_RES* pResult, MYSQL_ROW& row, MYSQL_FIELD* fields, uint32 numFields)
{
    m_columnNames.reserve(numFields);

    for (uint32 i = 0; i < numFields; ++i)
    {
        m_columnNames.emplace_back(fields[i].name);
        m_columnIndices[fields[i].name] = i;
    }

    uint64 numRows = mysql_num_rows(pResult);
    m_rows.reserve(numRows);

    while ((row = mysql_fetch_row(pResult)))
    {
        unsigned long* lengths = mysql_fetch_lengths(pResult);

        VariantVector& curRow = m_rows.emplace_back();
        curRow.reserve(numFields);

        for (uint32 i = 0; i < numFields; ++i)
        {
            curRow.push_back(ExtractFieldVariant(fields[i], row[i], lengths[i], row[i] == nullptr));
        }
    }
}

void DatabaseResult::ParseSTMT(MYSQL_STMT* pStmt, MYSQL_RES* pResult, MYSQL_ROW& row, MYSQL_FIELD* fields,
                               uint32 numFields)
{
    if (mysql_stmt_store_result(pStmt) != 0) // test
        return;

    PreparedParam prepParam(numFields); // umm used uint8 inside lol
    prepParam.Load(fields);

    mysql_stmt_bind_result(pStmt, prepParam.GetBinds().data());

    m_columnNames.reserve(numFields);
    for (uint32 i = 0; i < numFields; ++i)
    {
        m_columnNames.emplace_back(fields[i].name);
        m_columnIndices[fields[i].name] = i;
    }

    uint64 numRows = mysql_num_rows(pResult);
    m_rows.reserve(numRows);

    while ((mysql_stmt_fetch(pStmt) == 0))
    {
        VariantVector& curRow = m_rows.emplace_back();
        curRow.reserve(numFields);

        for (uint32 i = 0; i < numFields; ++i)
        {
            curRow.push_back(ExtractFieldVariant(fields[i], prepParam.GetBuffers()[i].data(), prepParam.GetLengths()[i],
                                                 prepParam.GetNulls()[i]));
        }
    }
}

Variant DatabaseResult::ExtractFieldVariant(MYSQL_FIELD& field, void* data, unsigned long length, bool isNull)
{
    if (isNull || !data)
        return Variant();

    switch (field.type)
    {
        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_LONGLONG:
        {
            return (field.flags & UNSIGNED_FLAG) ? Variant(ToUInt((const char*)data))
                                                 : Variant(ToInt((const char*)data));
        }

        case MYSQL_TYPE_DOUBLE:
        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_NEWDECIMAL:
        case MYSQL_TYPE_FLOAT:
        {
            return Variant(ToFloat((const char*)data));
        }

        case MYSQL_TYPE_BLOB:
        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_VAR_STRING:
        {
            return Variant(string((char*)data, length));
        }

        default:
        {
            LOGGER_LOG_WARN("We dont know how to parse %s type %d", field.name, field.type);
            return Variant();
        }
    }
}
