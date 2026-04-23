#include "PlayerDBTable.h"

QueryRequest PlayerDB::GetByMac(const string& mac, uint8 platformType, uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.AddData(mac, platformType);

    req.queryID = DB_PLAYER_GET_BY_MAC;
    return req;
}

QueryRequest PlayerDB::Create(const string& guestName, uint8 platformType, uint16 guestID, const string& mac, const string& ip, uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.AddData(guestName, platformType, guestID, mac, ip);

    req.queryID = DB_PLAYER_CREATE;
    return req;
}

QueryRequest PlayerDB::GetData(uint32 userID, uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.AddData(userID);

    req.queryID = DB_PLAYER_GET_DATA;
    return req;
}

QueryRequest PlayerDB::Save(uint32 userID, uint32 roleID, const string& inventoryData, uint32 skinColor, uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.AddData(roleID, inventoryData, userID, skinColor);

    req.queryID = DB_PLAYER_SAVE;
    return req;
}

QueryRequest PlayerDB::CountByIP(const string& ip, uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.AddData(ip);

    req.queryID = DB_PLAYER_COUNT_IP;
    return req;
}

QueryRequest PlayerDB::GetByVID(const string& vid, uint8 platformType, uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.AddData(vid, platformType);

    req.queryID = DB_PLAYER_GET_BY_VID;
    return req;
}

QueryRequest PlayerDB::GetByGID(const string& gid, uint8 platformType, uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.AddData(gid, platformType);

    req.queryID = DB_PLAYER_GET_BY_GID;
    return req;
}

QueryRequest PlayerDB::GetByHash(int32 hash, uint8 platformType, uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.AddData(hash, platformType);

    req.queryID = DB_PLAYER_GET_BY_HASH;
    return req;
}

QueryRequest PlayerDB::CountByGidMacIP(const string& gid, const string& mac, const string& ip, uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.AddData(gid, mac, ip);

    req.queryID = DB_PLAYER_COUNT_GID_MAC_IP;
    return req;
}

QueryRequest PlayerDB::CountByVidMacIP(const string& vid, const string& mac, const string& ip, uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.AddData(vid, mac, ip);

    req.queryID = DB_PLAYER_COUNT_VID_MAC_IP;
    return req;
}

QueryRequest PlayerDB::CountBySidMacIP(const string& sid, const string& mac, const string& ip, uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.AddData(sid, mac, ip);

    req.queryID = DB_PLAYER_COUNT_SID_MAC_IP;
    return req;
}

QueryRequest PlayerDB::CountByMacIP(const string& mac, const string& ip, uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.AddData(mac, ip);

    req.queryID = DB_PLAYER_COUNT_MAC_IP;
    return req;
}

QueryRequest PlayerDB::GrowIDExists(const string& growID, uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.AddData(growID);

    req.queryID = DB_PLAYER_GROWID_EXISTS;
    return req;
}

QueryRequest PlayerDB::GrowIDCreate(uint32 userID, const string& name, const string& pass, uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.AddData(name, pass, userID);

    req.queryID = DB_PLAYER_GROWID_CREATE;
    return req;
}

QueryRequest PlayerDB::GetByNameAndPass(const string& name, const string& pass, uint32 ownerID)
{
    QueryRequest req(ownerID);
    req.AddData(name, pass);

    req.queryID = DB_PLAYER_GET_BY_NAME_AND_PASS;
    return req;
}

void DatabasePlayerExec(DatabasePool* pPool, QueryRequest& req, bool preapred)
{
    if(req.queryID < 0) {
        DatabaseExec(pPool, "", req, QUERY_FLAG_RETURN_RESULT); // fail query
        return;
    }

    TableQuery& query = sQueryTable[req.queryID];

    if(preapred) {
        query.flags |= QUERY_FLAG_PREPARED;
    }

    DatabaseExec(pPool, query.query, req, query.flags);
}

void DatabasePlayerIdentifierExec(DatabasePool* pPool, uint32 userID, const string& mac, const string& vid, const string& sid, const string& rid, const string& gid, int32 hash, QueryRequest& req)
{
    string query = "UPDATE Players SET ";

    if(!mac.empty() && mac != "02:00:00:00:00:00") {
        query += "Mac = ?, ";
        req.AddData(mac);
    }

    if(!vid.empty()) {
        query += "VID = UNHEX(MD5(?)), ";
        req.AddData(vid);
    }

    if(!sid.empty()) {
        query += "SID = UNHEX(MD5(?)), ";
        req.AddData(sid);
    }

    if(!rid.empty()) {
        query += "RID = UNHEX(?), ";
        req.AddData(rid);
    }

    if(!gid.empty()) {
        query += "GID = UNHEX(MD5(?)), ";
        req.AddData(gid);
    }

    query += "Hash = ? WHERE ID = ?;";
    req.AddData(hash, userID);

    DatabaseExec(pPool, query, req, QUERY_FLAG_NONE);
}
