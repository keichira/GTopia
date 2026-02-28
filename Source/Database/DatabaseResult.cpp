#include "DatabaseResult.h"
#include "PreparedParam.h"
#include <ctime>
#include "../IO/Log.h"
#include "../Utils/StringUtils.h"

DatabaseResult::DatabaseResult()
{
}

bool DatabaseResult::Parse(MYSQL* pConnection, MYSQL_STMT* pStmt)
{
    if(!pConnection) {
        return false;
    }

    MYSQL_RES* pResult = pStmt ? mysql_stmt_result_metadata(pStmt) : mysql_store_result(pConnection);
    if(!pResult) {
        return false;
    }

    MYSQL_ROW row;
    uint32 numFields = mysql_num_fields(pResult);
    MYSQL_FIELD* fields = mysql_fetch_field(pResult);

    if(pStmt) {
        ParseSTMT(pStmt, pResult, row, fields, numFields);
    }
    else {
        ParseNormal(pResult, row, fields, numFields);
    }

    mysql_free_result(pResult);
    return true;
}

uint32 DatabaseResult::GetSizeOfField(MYSQL_FIELD &field)
{
    switch(field.type) {
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
    while((row = mysql_fetch_row(pResult))) {
        unsigned long* lengths = mysql_fetch_lengths(pResult);
        VariantMap& varMap = m_results.emplace_back();

        for(uint32 i = 0; i < numFields; ++i) {
            ParseAndInsertField(fields[i], row[i], lengths[i], row[i] == nullptr, varMap);
        }
    }
}

void DatabaseResult::ParseSTMT(MYSQL_STMT* pStmt, MYSQL_RES* pResult, MYSQL_ROW& row, MYSQL_FIELD* fields, uint32 numFields)
{
    PreparedParam prepParam(numFields); // umm used uint8 inside lol
    prepParam.Load(fields);

    mysql_stmt_bind_result(pStmt, prepParam.GetBinds().data());

    while((mysql_stmt_fetch(pStmt) == 0)) {
        VariantMap& varMap = m_results.emplace_back();

        for(uint32 i = 0; i < numFields; ++i) {
            ParseAndInsertField(fields[i], prepParam.GetBuffers()[i].data(), prepParam.GetLengths()[i], prepParam.GetNulls()[i], varMap);
        }
    }
}

void DatabaseResult::ParseAndInsertField(MYSQL_FIELD& field, void* data, unsigned long length, bool isNull, VariantMap& variantMap)
{
    Variant var;

    switch(field.type) {
        case MYSQL_TYPE_TINY:
        case MYSQL_TYPE_SHORT:
        case MYSQL_TYPE_LONG:
        case MYSQL_TYPE_LONGLONG: {
            var = (isNull ? (uint32)0 : 
            (field.flags & UNSIGNED_FLAG ?
                      ToUInt((const char*)data) :
                      ToInt((const char*)data)));
            break;
        }

        case MYSQL_TYPE_FLOAT:
        case MYSQL_TYPE_DOUBLE: 
        case MYSQL_TYPE_DECIMAL:
        case MYSQL_TYPE_NEWDECIMAL:{
            var = (isNull ? (float)0 : ToFloat((const char*)data));
            break;
        }

        case MYSQL_TYPE_BLOB:
        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_VAR_STRING: {
            var = (isNull ? "" : string((char*)data, length));
            break;
        }

        default: {
            LOGGER_LOG_WARN("We dont know how to parse %s type %d", field.name, field.type);
        }
    }

    variantMap.insert_or_assign(field.name, std::move(var));
}
