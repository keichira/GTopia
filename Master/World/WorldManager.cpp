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

void WorldManager::OnHandleDatabase(QueryTaskResult&& result)
{
    switch(result.extraData[0].GetINT()) {
        case WORLD_DB_STATE_CHECK: {
            CheckWorldExists(std::move(result));
            break;
        }

        case WORLD_DB_STATE_CREATE: {
            CreateWorld(std::move(result));
            break;
        }
    }

    result.Destroy();
}

void WorldManager::HandleWorldInit(bool success, uint32 worldID)
{
    auto it = m_worldSession.find(worldID);
    if(it == m_worldSession.end()) {
        return;
    }

    WorldSession* pWorld = it->second;
    ServerManager* pServerMgr = GetServerManager();

    if(!success) {
        VariantVector data(3);
        data[0] = TCP_PACKET_WORLD_INIT;
        data[1] = false;

        for(auto& pending : pWorld->pendingPlayers) {
            data[2] = pending.playerNetID;
            pServerMgr->SendPacketRaw(pending.serverID, data);
        }

        SAFE_DELETE(pWorld);
        m_worldSession.erase(worldID);
        return;
    }

    pWorld->state = WORLD_STATE_ON;
    ServerInfo* pServer = pServerMgr->GetServerByID(pWorld->serverID);

    VariantVector data(6);
    data[0] = TCP_PACKET_WORLD_INIT;
    data[1] = (uint32)pWorld->serverID;
    /*data[2]*/
    data[3] = worldID;
    data[4] = pServer->wanIP;
    data[5] = pServer->serverID;

    for(auto& pending : pWorld->pendingPlayers) {
        data[2] = pending.playerNetID;
        pServerMgr->SendPacketRaw(pending.serverID, data);
    }
    pWorld->pendingPlayers.clear();
}

void WorldManager::ManagePlayerJoin(uint16 serverID, int32 playerNetID, const string& worldName)
{
    WorldSession* pWorld = GetWorldByName(worldName);

    if(!pWorld) {
        string upperWorldName = ToUpper(worldName);

        QueryRequest req = MakeWorldExistsByName(upperWorldName, GetNetID());
        req.extraData.resize(4);
        req.extraData[0] = WORLD_DB_STATE_CHECK;
        req.extraData[1] = (uint32)serverID;
        req.extraData[2] = playerNetID;
        req.extraData[3] = upperWorldName;

        DatabaseWorldExistsByName(GetContext()->GetDatabasePool(), req);
        return;
    }

    if(pWorld->state == WORLD_STATE_LOADING) {
        pWorld->pendingPlayers.emplace_back(WorldPendingPlayer{ serverID, playerNetID });
    }
    else if(pWorld->state == WORLD_STATE_ON) {
        
    }
}

void WorldManager::CheckWorldExists(QueryTaskResult&& result)
{
    if(!result.result) {
        // woah cool
        // send error
        return;
    }

    if(result.result->GetRowCount() == 0) {
        QueryRequest req = MakeWorldCreate(result.extraData[3].GetString(), GetNetID());
        req.extraData = std::move(result.extraData);
        req.extraData[0] = WORLD_DB_STATE_CREATE;

        DatabaseWorldCreate(GetContext()->GetDatabasePool(), req);
        return;
    }

    ServerManager* pServerMgr = GetServerManager();

    ServerInfo* pServer = pServerMgr->GetBestServer();
    WorldSession* pWorld = new WorldSession();

    pWorld->worldID = result.result->GetField("ID", 0).GetUINT();
    pWorld->state = WORLD_STATE_LOADING;
    pWorld->serverID = pServer->serverID;
    pWorld->worldName = result.extraData[3].GetString();
    pWorld->pendingPlayers.emplace_back(WorldPendingPlayer{ (uint16)result.extraData[1].GetUINT(), result.extraData[2].GetINT() });

    VariantVector packet(2);
    packet[0] = TCP_PACKET_WORLD_INIT;
    packet[1] = result.extraData[3].GetString();

    pServerMgr->SendPacketRaw(pServer->serverID, packet);
}

void WorldManager::CreateWorld(QueryTaskResult&& result)
{
    ServerManager* pServerMgr = GetServerManager();
    ServerInfo* pServer = pServerMgr->GetBestServer();

    WorldSession* pWorld = new WorldSession();
    pWorld->worldID = result.increment;
    pWorld->state = WORLD_STATE_LOADING;
    pWorld->serverID = pServer->serverID;
    pWorld->pendingPlayers.emplace_back(WorldPendingPlayer{ (uint16)result.extraData[1].GetUINT(), result.extraData[2].GetINT() });

    m_worldSession.insert_or_assign(pWorld->worldID, pWorld);

    VariantVector packet(2);
    packet[0] = TCP_PACKET_WORLD_INIT;
    packet[1] = result.extraData[3].GetString();

    pServerMgr->SendPacketRaw(pServer->serverID, packet);
}

WorldSession* WorldManager::GetWorldByName(const string& worldName)
{
    string searchName = ToLower(worldName);
    for(auto& [_, pWorld] : m_worldSession) {
        if(ToLower(pWorld->worldName) == searchName) {
            return pWorld;
        }
    }

    return nullptr;
}

WorldManager* GetWorldManager() { return WorldManager::GetInstance(); }