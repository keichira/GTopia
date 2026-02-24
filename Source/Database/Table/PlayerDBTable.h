#pragma once

#include "../DatabasePool.h"

void DatabaseGetPlayerByMac(DatabasePool* pPool, QueryRequest& req, bool prepared = false);
QueryRequest MakePlayerByMacReq(const string& mac, const string& rid, uint8 platformType, int32 ownerID);

void DatabasePlayerCreate(DatabasePool* pPool, QueryRequest& req, bool prepared = false);
QueryRequest MakePlayerCreateReq(const string& guestName, uint8 platformType, uint16 guestID, const string& mac, const string& rid, int32 ownerID);

//void DatabasePlayerSetIdentifiers(DatabasePool* pPool, QueryRequest& req, bool prepared = false);