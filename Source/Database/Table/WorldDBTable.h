#pragma once

#include "../DatabasePool.h"

void DatabaseWorldExistsByName(DatabasePool* pPool, QueryRequest& req, bool prepared = false);
QueryRequest MakeWorldExistsByName(const string& worldName, int32 ownerID);

void DatabaseWorldCreate(DatabasePool* pPool, QueryRequest& req, bool prepared = false);
QueryRequest MakeWorldCreate(const string& worldName, int32 ownerID);

void DatabaseGetWorldData(DatabasePool* pPool, QueryRequest& req, bool prepared = false);
QueryRequest MakeGetWorldData(const string& worldName, int32 ownerID);