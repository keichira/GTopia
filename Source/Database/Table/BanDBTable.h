#pragma once

#include "../DatabasePool.h"

// clang-format off
static TableQuery sBanQueryTable[] =
{
    {"", QUERY_FLAG_RETURN_RESULT}
};
// clang-format on

enum eBanDBQuery
{
};

namespace BanDB
{

} // namespace BanDB

void DatabaseBanExec(DatabasePool* pPool, QueryRequest& req, bool preapred = false, bool bulk = false);