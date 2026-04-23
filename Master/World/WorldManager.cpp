#include "WorldManager.h"
#include "Utils/StringUtils.h"
#include "Database/Table/WorldDBTable.h"
#include "../Context.h"
#include "../Server/ServerManager.h"

WorldManager::WorldManager()
{
}

WorldManager::~WorldManager()
{
}

void WorldManager::HandleWorldInit(VariantVector&& result)
{
    if(result.size() < 3) {
        return;
    }

    int32 initResult = result[1].GetINT();
    uint32 worldID = result[2].GetUINT();

    WorldSession* pWorld = GetWorldByID(worldID);
    if(!pWorld) {
        return;
    }

    ServerManager* pServerMgr = GetServerManager();

    if(initResult != TCP_RESULT_OK) {
        for(auto& pending : pWorld->pendingPlayers) {
            pServerMgr->SendWorldPlayerFailPacket(pending.playerNetID, pending.serverID);
        }

        m_worldSessions.erase(worldID);
        return;
    }

    pWorld->state = WORLD_STATE_ON;

    ServerInfo* pServer = pServerMgr->GetServerByID(pWorld->serverID);
    if(!pServer) {
        return;
    }

    for(auto& pending : pWorld->pendingPlayers) {
        pServerMgr->SendWorldPlayerSuccessPacket(
            pending.playerNetID,
            pWorld->serverID,
            worldID,
            pServer->wanIP,
            pServer->port,
            pending.serverID
        );
    }

    pWorld->pendingPlayers.clear();
}

void WorldManager::CheckWorldExistCB(QueryTaskResult&& result)
{
    Variant* pServerID = result.GetExtraData(0);
    Variant* pPlayerID = result.GetExtraData(1);

    if(!pServerID || !pPlayerID) {
        return;
    }

    Variant* pWorldName = result.GetExtraData(2);

    if(!result.result || !pWorldName) {
        GetServerManager()->SendWorldPlayerFailPacket(pPlayerID->GetINT(), pServerID->GetUINT());
        return;
    }

    if(result.result->GetRowCount() > 0) {
        Variant* pID = result.result->GetFieldSafe("ID", 0);
        if(!pID) {
            GetServerManager()->SendWorldPlayerFailPacket(pPlayerID->GetINT(), pServerID->GetUINT());
            return;
        }

        GetWorldManager()->CreateWorldSessionAndNotice(pID->GetUINT(), pWorldName->GetString(), pPlayerID->GetINT(), pServerID->GetUINT());
        return;
    }

    QueryRequest req = WorldDB::Create(pWorldName->GetString());
    req.extraData = std::move(result.extraData);
    req.callback = &WorldManager::CreateWorldCB;

    DatabaseWorldExec(GetContext()->GetDatabasePool(), req);
}

void WorldManager::CreateWorldCB(QueryTaskResult&& result)
{
    Variant* pServerID = result.GetExtraData(0);
    Variant* pPlayerID = result.GetExtraData(1);

    if(!pServerID || !pPlayerID) {
        return;
    }

    Variant* pWorldName = result.GetExtraData(2);

    if(result.increment == 0 || !pWorldName) {
        GetServerManager()->SendWorldPlayerFailPacket(pPlayerID->GetINT(), pServerID->GetUINT());
        return;
    }

    GetWorldManager()->CreateWorldSessionAndNotice(result.increment, pWorldName->GetString(), pPlayerID->GetINT(), pServerID->GetUINT());
}

void WorldManager::HandlePlayerJoinRequest(VariantVector&& result)
{
    if(result.size() < 4) {
        return;
    }

    uint32 serverID = result[1].GetUINT();
    int32 playerNetID = result[2].GetINT();
    string upperWorldName = ToUpper(result[3].GetString());

    WorldSession* pWorld = GetWorldByName(upperWorldName);
    if(!pWorld) {
        QueryRequest req = WorldDB::ExistsByName(upperWorldName);
        req.AddExtraData(serverID, playerNetID, upperWorldName);
        req.callback = &WorldManager::CheckWorldExistCB;

        DatabaseWorldExec(GetContext()->GetDatabasePool(), req);
    }
    else if(pWorld->state == WORLD_STATE_LOADING) {
        pWorld->AddPending(serverID, playerNetID);
    }
    else if(pWorld->state == WORLD_STATE_ON) {
        ServerInfo* pServer = GetServerManager()->GetServerByID(pWorld->serverID);
        if(!pServer) {
            GetServerManager()->SendWorldPlayerFailPacket(playerNetID, serverID);
            return;
        }

        GetServerManager()->SendWorldPlayerSuccessPacket(
            playerNetID,
            pWorld->serverID,
            pWorld->worldID,
            pServer->wanIP,
            pServer->port,
            serverID
        );
    }
}

void WorldManager::CreateWorldSessionAndNotice(uint32 worldID, const string& worldName, int32 playerNetID, uint32 serverID)
{
    ServerManager* pServerMgr = GetServerManager();
    ServerInfo* pServer = pServerMgr->GetBestGameServer();
    if(!pServer) {
        pServerMgr->SendWorldPlayerFailPacket(playerNetID, serverID);
        return;
    }

    WorldSession worldSession;

    worldSession.worldID = worldID;
    worldSession.state = WORLD_STATE_LOADING;
    worldSession.serverID = pServer->serverID;
    worldSession.worldName = worldName;
    worldSession.AddPending(serverID, playerNetID);

    m_worldSessions.insert_or_assign(worldSession.worldID, worldSession);
    pServerMgr->SendWorldInitPacket(worldName, pServer->serverID);
}

WorldSession* WorldManager::GetWorldByName(const string& worldName)
{
    string searchName = ToLower(worldName);
    for(auto& [_, worldSession] : m_worldSessions) {
        if(ToLower(worldSession.worldName) == searchName) {
            return &worldSession;
        }
    }

    return nullptr;
}

WorldSession* WorldManager::GetWorldByID(uint32 worldID)
{
    auto it = m_worldSessions.find(worldID);
    if(it != m_worldSessions.end()) {
        return &it->second;
    }

    return nullptr;
}

void WorldManager::RemoveWorldsWithServerID(uint32 serverID)
{
    for(auto it = m_worldSessions.begin(); it != m_worldSessions.end();) {
        if(it->second.serverID == serverID) {
            it = m_worldSessions.erase(it);
            continue;
        }

        ++it;
    }
}

WorldManager* GetWorldManager() { return WorldManager::GetInstance(); }