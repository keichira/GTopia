#include "SetRole.h"
#include "Player/RoleManager.h"
#include "../../Context.h"
#include "Database/Table/PlayerDBTable.h"
#include "../../Player/PlayerManager.h"
#include "../../Server/ServerManager.h"

const TelnetCommandInfo& SetRole::GetInfo()
{
    static TelnetCommandInfo info =
    {
        "/setrole <userID> <roleID>",
        "Set player's RoleID",
        4,
        {
            "setrole"_hash
        }
    };

    return info;
}

void SetRoleUpdateRoleCB(QueryTaskResult&& result)
{
    TelnetClient* pNetClient = GetTelnetServer()->GetClientByNetID(result.ownerID);
    if(!pNetClient) {
        return;
    }

    if(result.status != QUERY_STATUS_OK) {
        pNetClient->SendMessage("Failed update role id for user.", true);
    }
    else {
        pNetClient->SendMessage("Successfully updated player's role", true);

        Variant* pRoleID = result.GetExtraData(0);
        Variant* pUserID = result.GetExtraData(1);

        if(pRoleID && pUserID) 
        {
            PlayerSession* pPlayerSession = GetPlayerManager()->GetSessionByID(pUserID->GetUINT());
            if(pPlayerSession) 
            {
                ServerInfo* pServer = GetServerManager()->GetServerByID(pPlayerSession->serverID);
                if(pServer)
                {
                    GetServerManager()->SendCommandSetRole(pServer, pPlayerSession->userID, pRoleID->GetUINT());
                }
            }
        }
    }

    pNetClient->SetBusy(false);
}

void SetRoleCheckPlayerCB(QueryTaskResult&& result) 
{
    TelnetClient* pNetClient = GetTelnetServer()->GetClientByNetID(result.ownerID);
    if(!pNetClient) {
        return;
    }

    if(!result.result) {
        pNetClient->SendMessage("Error happened while checking user.", true);
        pNetClient->SetBusy(false);
        return;
    }

    if(result.result->GetRowCount() > 0) {
        Variant* pRoleID = result.GetExtraData(0);
        Variant* pUserID = result.GetExtraData(1);

        if(!pRoleID || !pUserID) {
            pNetClient->SendMessage("Oops! Something went wrong.", true);
            pNetClient->SetBusy(false);
            return;
        }

        QueryRequest req = PlayerDB::SetRoleByID(pRoleID->GetUINT(), pUserID->GetUINT(), pNetClient->GetNetID());
        req.AddExtraData(pRoleID->GetUINT(), pUserID->GetUINT());
        req.callback = &SetRoleUpdateRoleCB;

        DatabasePlayerExec(GetContext()->GetDatabasePool(), req);
    }
    else {
        pNetClient->SendMessage("User not found.", true);
        pNetClient->SetBusy(false);
        return;
    }
}

void SetRole::Execute(TelnetClient* pNetClient, std::vector<string>& args)
{
    if(!pNetClient || args.empty() || !CheckPerm(pNetClient)) {
        return;
    }
    
    if(args.size() < 3) {
        SendUsage(pNetClient);
        return;
    }

    uint32 userID = 0;
    if(ToUInt(args[1], userID) != TO_INT_SUCCESS) {
        pNetClient->SendMessage("UserID must be number.", true);
        return;
    }

    if(userID == 0) {
        pNetClient->SendMessage("User not found.", true);
        return;
    }

    uint32 roleID = 0;
    if(ToUInt(args[2], roleID) != TO_INT_SUCCESS) {
        pNetClient->SendMessage("RoleID must be number.", true);
        return;
    }

    Role* pRole = GetRoleManager()->GetRole(roleID);
    if(!pRole) {
        pNetClient->SendMessage("RoleID not found.", true);
        return;
    }

    pNetClient->SetBusy(true);

    QueryRequest req = PlayerDB::ExistByID(userID, pNetClient->GetNetID());
    req.AddExtraData(roleID, userID);
    req.callback = &SetRoleCheckPlayerCB;

    DatabasePlayerExec(GetContext()->GetDatabasePool(), req);
}
