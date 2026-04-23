#pragma once

#include "../DatabasePool.h"

static TableQuery sQueryTable[] =
{
    {"SELECT ID FROM Players WHERE Name = '' AND Mac = ? AND PlatformType = ? LIMIT 1;", QUERY_FLAG_RETURN_RESULT},
    {"INSERT INTO Players (GuestName, PlatformType, GuestID, Mac, IP, CreationDate, LastSeenTime) VALUES (?, ?, ?, ?, ?, SYSDATE(), NOW());", QUERY_FLAG_RETURN_INCREMENT},
    {"SELECT GuestID, Name, SkinColor, RoleID, HEX(Inventory) AS Inventory FROM Players WHERE ID = ?;", QUERY_FLAG_RETURN_RESULT},
    {"UPDATE Players SET LastSeenTime = NOW(), RoleID = ?, Inventory = UNHEX(?), SkinColor = ? WHERE ID = ?;", QUERY_FLAG_NONE},
    {"SELECT ID FROM Players WHERE IP = ?;", QUERY_FLAG_RETURN_RESULT},
    {"SELECT ID FROM Players WHERE Name = '' AND VID = UNHEX(MD5(?)) AND PlatformType = ?;", QUERY_FLAG_RETURN_RESULT},
    {"SELECT ID FROM Players WHERE Name = '' AND GID = UNHEX(MD5(?)) AND PlatformType = ?;", QUERY_FLAG_RETURN_RESULT},
    {"SELECT ID FROM Players WHERE Name = '' AND Hash = ? AND PlatformType = ?;", QUERY_FLAG_RETURN_RESULT},
    {"SELECT (SELECT COUNT(*) FROM Players WHERE IP = ?)  AS ip_count, (SELECT COUNT(*) FROM Players WHERE MAC = ?) AS mac_count, (SELECT COUNT(*) FROM Players WHERE GID = ?) AS gid_count;", QUERY_FLAG_RETURN_RESULT},
    {"SELECT (SELECT COUNT(*) FROM Players WHERE IP = ?)  AS ip_count, (SELECT COUNT(*) FROM Players WHERE MAC = ?) AS mac_count, (SELECT COUNT(*) FROM Players WHERE VID = ?) AS vid_count;", QUERY_FLAG_RETURN_RESULT},
    {"SELECT (SELECT COUNT(*) FROM Players WHERE IP = ?)  AS ip_count, (SELECT COUNT(*) FROM Players WHERE MAC = ?) AS mac_count, (SELECT COUNT(*) FROM Players WHERE SID = ?) AS sid_count;", QUERY_FLAG_RETURN_RESULT},
    {"SELECT (SELECT COUNT(*) FROM Players WHERE IP = ?)  AS ip_count, (SELECT COUNT(*) FROM Players WHERE MAC = ?) AS mac_count;", QUERY_FLAG_RETURN_RESULT},
    {"SELECT ID FROM Players WHERE Name = ? LIMIT 1;", QUERY_FLAG_RETURN_RESULT},
    {"UPDATE Players SET Name = ?, Password = UNHEX(MD5(?)) WHERE ID = ?;"},
    {"SELECT ID FROM Players WHERE Name = ? AND Password = UNHEX(MD5(?));", QUERY_FLAG_RETURN_RESULT}
};

enum ePlayerDBQuery
{
    DB_PLAYER_GET_BY_MAC,
    DB_PLAYER_CREATE,
    DB_PLAYER_GET_DATA,
    DB_PLAYER_SAVE,
    DB_PLAYER_COUNT_IP,
    DB_PLAYER_GET_BY_VID,
    DB_PLAYER_GET_BY_GID,
    DB_PLAYER_GET_BY_HASH,
    DB_PLAYER_COUNT_GID_MAC_IP,
    DB_PLAYER_COUNT_VID_MAC_IP,
    DB_PLAYER_COUNT_SID_MAC_IP,
    DB_PLAYER_COUNT_MAC_IP,
    DB_PLAYER_GROWID_EXISTS,
    DB_PLAYER_GROWID_CREATE,
    DB_PLAYER_GET_BY_NAME_AND_PASS
};

namespace PlayerDB
{
    QueryRequest GetByMac(const string& mac, uint8 platformType, uint32 ownerID = 0);
    QueryRequest Create(const string& guestName, uint8 platformType, uint16 guestID, const string& mac, const string& ip, uint32 ownerID = 0);
    QueryRequest GetData(uint32 userID, uint32 ownerID = 0);
    QueryRequest Save(uint32 userID, uint32 roleID, const string& inventoryData, uint32 skinColor, uint32 ownerID = 0);
    QueryRequest CountByIP(const string& ip, uint32 ownerID = 0);

    QueryRequest GetByVID(const string& vid, uint8 platformType, uint32 ownerID = 0);
    QueryRequest GetByGID(const string& gid, uint8 platformType, uint32 ownerID = 0);
    QueryRequest GetByHash(int32 hash, uint8 platformType, uint32 ownerID = 0);

    QueryRequest CountByGidMacIP(const string& gid, const string& mac, const string& ip, uint32 ownerID = 0);
    QueryRequest CountByVidMacIP(const string& vid, const string& mac, const string& ip, uint32 ownerID = 0);
    QueryRequest CountBySidMacIP(const string& sid, const string& mac, const string& ip, uint32 ownerID = 0);
    QueryRequest CountByMacIP(const string& mac, const string& ip, uint32 ownerID = 0);

    QueryRequest GrowIDExists(const string& growID, uint32 ownerID = 0);
    QueryRequest GrowIDCreate(uint32 userID, const string& name, const string& pass, uint32 ownerID = 0);

    QueryRequest GetByNameAndPass(const string& name, const string& pass, uint32 ownerID = 0);
}

void DatabasePlayerExec(DatabasePool* pPool, QueryRequest& req, bool preapred = false);
void DatabasePlayerIdentifierExec(DatabasePool* pPool, uint32 userID, const string& mac, const string& vid, const string& sid, const string& rid, const string& gid, int32 hash, QueryRequest& req);