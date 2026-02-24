#include "PlayerDBTable.h"

void DatabaseGetPlayerByMac(DatabasePool* pPool, QueryRequest& req, bool prepared)
{
    uint16 flags = QUERY_FLAG_RETURN_RESULT;
    if(prepared) {
        flags |= QUERY_FLAG_PREPARED;
    }

    DatabaseExec(
        pPool, 
        "SELECT ID FROM Players WHERE Name = '' AND Mac = ? AND RID = UNHEX(?) AND PlatformType = ? LIMIT 1;",
        req,
        flags
    );
}

QueryRequest MakePlayerByMacReq(const string& mac, const string& rid, uint8 platformType, int32 ownerID)
{
    QueryRequest req;
    req.ownerID = ownerID;

    req.data.resize(3);
    req.data[0] = mac;
    req.data[1] = rid;
    req.data[2] = platformType;
    return req;
}

void DatabasePlayerCreate(DatabasePool* pPool, QueryRequest& req, bool prepared)
{
    uint16 flags = QUERY_FLAG_RETURN_INCREMENT;
    if(prepared) {
        flags |= QUERY_FLAG_PREPARED;
    }

    DatabaseExec(
        pPool, 
        "INSERT INTO Players (GuestName, PlatformType, GuestID, Mac, RID, CreationDate, LastSeenTime) VALUES (?, ?, ?, ?, UNHEX(?), SYSDATE(), NOW());",
        req,
        flags
    );
}

QueryRequest MakePlayerCreateReq(const string& guestName, uint8 platformType, uint16 guestID, const string& mac, const string& rid, int32 ownerID)
{
    QueryRequest req;
    req.ownerID = ownerID;

    req.data.resize(5);
    req.data[0] = guestName;
    req.data[1] = platformType;
    req.data[2] = guestID;
    req.data[3] = mac;
    req.data[4] = rid;
    return req;
}

/*void DatabasePlayerSetIdentifiers(DatabasePool* pPool, QueryRequest& req, bool prepared)
{
    uint16 flags = QUERY_FLAG_NONE;
    if(prepared) {
        flags |= QUERY_FLAG_PREPARED;
    }

    DatabaseExec(
        pPool, 
        "UPDATE Players SET Mac = ?, RID = UNHEX(?) WHERE ID = ?;",
        req,
        flags
    );
}*/
