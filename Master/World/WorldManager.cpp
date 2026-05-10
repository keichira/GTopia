#include "WorldManager.h"
#include "Utils/StringUtils.h"
#include "Database/Table/WorldDBTable.h"
#include "../Context.h"
#include "../Server/ServerManager.h"
#include "../Player/PlayerManager.h"

WorldManager::WorldManager()
{
}

WorldManager::~WorldManager()
{
}

void WorldManager::HandlePlayerJoinRequest(ServerInfo* pServer, VariantVector&& result)
{
    if(!pServer || result.size() < 3) {
        return;
    }

    uint32 userID = result[1].GetUINT();
    string worldName = ToUpper(result[2].GetString());

    WorldSession* pWorld = GetWorldByName(worldName);
    if(pWorld) {
        if(pWorld->state == WORLD_STATE_DELETE) {
            FailPlayerJoin(pServer, userID, "Unable to move you to this world, please try again in a few seconds.");
            return;
        }

        if(pWorld->state == WORLD_STATE_LOADING) {
            AttachPending(pWorld, pServer->serverID, userID);
            return;
        }

        RoutePlayerToExistingWorld(pServer, userID, *pWorld);
        return;
    }

    QueryRequest req = WorldDB::ExistsByName(worldName);
    req.AddExtraData((uint32)pServer->serverID, userID, worldName);
    req.callback = &WorldManager::CheckWorldExistCB;
    DatabaseWorldExec(GetContext()->GetDatabasePool(), req);
}

void WorldManager::CheckWorldExistCB(QueryTaskResult&& result)
{
    Variant* pServerID = result.GetExtraData(0);
    Variant* pPlayerID = result.GetExtraData(1);
    Variant* pWorldName = result.GetExtraData(2);

    if(!pServerID || !pPlayerID || !pWorldName)
        return;

    ServerInfo* pServer = GetServerManager()->GetServerByID(pServerID->GetUINT());
    if(!pServer)
        return;

    if(!result.result)
    {
        GetServerManager()->SendWorldPlayerFailPacket(pServer, pPlayerID->GetUINT(), "");
        return;
    }

    if(result.result->GetRowCount() > 0)
    {
        Variant* pID = result.result->GetFieldSafe("ID", 0);
        if(!pID)
        {
            GetServerManager()->SendWorldPlayerFailPacket(pServer, pPlayerID->GetUINT(), "");
            return;
        }

        GetWorldManager()->CreateWorldSessionAndNotice(
            NetEntity(ENTITY_TYPE_WORLD).GetNetID(),
            pID->GetUINT(),
            pWorldName->GetString(),
            pPlayerID->GetUINT(),
            pServerID->GetUINT()
        );
        return;
    }

    QueryRequest req = WorldDB::Create(pWorldName->GetString());
    req.AddExtraData((uint32)pServer->serverID, pPlayerID->GetUINT(), pWorldName->GetString());
    req.callback = &WorldManager::CreateWorldCB;
    DatabaseWorldExec(GetContext()->GetDatabasePool(), req);
}

void WorldManager::CreateWorldCB(QueryTaskResult&& result)
{
    Variant* pServerID  = result.GetExtraData(0);
    Variant* pPlayerID  = result.GetExtraData(1);
    Variant* pWorldName = result.GetExtraData(2);

    if(!pServerID || !pPlayerID || !pWorldName)
        return;

    ServerInfo* pServer = GetServerManager()->GetServerByID(pServerID->GetUINT());
    if(!pServer)
        return;

    if(result.increment == 0)
    {
        GetServerManager()->SendWorldPlayerFailPacket(pServer, pPlayerID->GetUINT(), "");
        return;
    }

    GetWorldManager()->CreateWorldSessionAndNotice(
        NetEntity(ENTITY_TYPE_WORLD).GetNetID(),
        result.increment,
        pWorldName->GetString(),
        pPlayerID->GetUINT(),
        pServerID->GetUINT()
    );
}

void WorldManager::CreateWorldSessionAndNotice(uint32 instanceID, uint32 databaseID, const string& worldName, uint32 playerUserID, uint32 sourceServerID)
{
    ServerInfo* pTargetServer = GetServerManager()->GetBestGameServer();
    if(!pTargetServer)
    {
        ServerInfo* pSourceServer = GetServerManager()->GetServerByID(sourceServerID);
        if(pSourceServer)
        {
            GetServerManager()->SendWorldPlayerFailPacket(pSourceServer, playerUserID, "");
        }
        return;
    }

    WorldSession world;
    world.instanceID = instanceID;
    world.databaseID = databaseID;
    world.serverID = pTargetServer->serverID;
    world.state = WORLD_STATE_LOADING;
    world.worldName = worldName;

    PendingTransfer pending;
    pending.userID = playerUserID;
    pending.sourceServerID = sourceServerID;
    pending.targetServerID = pTargetServer->serverID;
    pending.worldDatabaseID = databaseID;
    pending.worldInstanceID = instanceID;
    pending.worldName = worldName;
    world.waitingPlayers.push_back(pending);

    m_worldSessionsByInstance.insert_or_assign(instanceID, world);
    m_worldInstanceByName.insert_or_assign(worldName, instanceID);

    GetServerManager()->SendWorldInitPacket(pTargetServer, worldName, instanceID, databaseID);
}

void WorldManager::HandleWorldInit(VariantVector&& result)
{
    if(result.size() < 3)
        return;

    int32 initResult = result[1].GetINT();
    uint32 instanceID = result[2].GetUINT();

    WorldSession* pWorld = GetWorldByInstanceID(instanceID);
    if(!pWorld)
        return;

    ServerManager* pServerMgr = GetServerManager();
    if(initResult != TCP_RESULT_OK)
    {
        for(auto& pending : pWorld->waitingPlayers)
        {
            ServerInfo* pSourceServer = pServerMgr->GetServerByID(pending.sourceServerID);
            if(pSourceServer)
            {
                pServerMgr->SendWorldPlayerFailPacket(pSourceServer, pending.userID, "");
            }
        }

        m_worldSessionsByInstance.erase(instanceID);
        m_worldInstanceByName.erase(pWorld->worldName);
        return;
    }

    pWorld->state = WORLD_STATE_READY;

    ServerInfo* pTargetServer = pServerMgr->GetServerByID(pWorld->serverID);
    if(!pTargetServer)
        return;

    for(auto& pending : pWorld->waitingPlayers)
    {
        PlayerSession* pSession = GetPlayerManager()->GetSessionByID(pending.userID);
        if(!pSession)
            continue;

        pSession->worldInstanceID = pWorld->instanceID;
        pSession->serverID = pWorld->serverID;

        ServerInfo* pSourceServer = pServerMgr->GetServerByID(pending.sourceServerID);
        if(!pSourceServer)
            continue;

        pServerMgr->SendWorldPlayerSuccessPacket(
            pSourceServer,
            pending.userID,
            pWorld->serverID,
            pWorld->instanceID,
            pTargetServer->wanIP,
            pTargetServer->port
        );
    }

    pWorld->waitingPlayers.clear();
}

WorldSession* WorldManager::GetWorldByName(const string& worldName)
{
    auto it = m_worldInstanceByName.find(worldName);
    if(it == m_worldInstanceByName.end())
        return nullptr;

    return GetWorldByInstanceID(it->second);
}

WorldSession* WorldManager::GetWorldByInstanceID(uint32 instanceID)
{
    auto it = m_worldSessionsByInstance.find(instanceID);
    if(it != m_worldSessionsByInstance.end()) 
        return &it->second;

    return nullptr;
}

void WorldManager::EndSessionsByServerID(uint32 serverID)
{
    for(auto it = m_worldSessionsByInstance.begin(); it != m_worldSessionsByInstance.end();) {
        if(it->second.serverID == serverID) {
            m_worldInstanceByName.erase(it->second.worldName);
            it = m_worldSessionsByInstance.erase(it);
            continue;
        }

        ++it;
    }
}

void WorldManager::RoutePlayerToExistingWorld(ServerInfo* pSourceServer, uint32 userID, WorldSession& world)
{
    if(!pSourceServer)
        return;

    ServerManager* pServerMgr = GetServerManager();
    if(!pServerMgr)
        return;

    ServerInfo* pTargetServer = pServerMgr->GetServerByID(world.serverID);
    if(!pTargetServer)
    {
        FailPlayerJoin(pSourceServer, userID, "Target server is offline.");
        return;
    }

    PlayerSession* pSession = GetPlayerManager()->GetSessionByID(userID);
    if(pSession)
    {
        pSession->serverID = world.serverID;
        pSession->worldInstanceID = world.instanceID;
    }

    pServerMgr->SendWorldPlayerSuccessPacket(
        pSourceServer,
        userID,
        world.serverID,
        world.instanceID,
        pTargetServer->wanIP,
        pTargetServer->port
    );
}

void WorldManager::AttachPending(WorldSession* pWorld, uint32 sourceServerID, uint32 userID)
{
    if(!pWorld)
        return;

    for(auto& pending : pWorld->waitingPlayers) 
    {
        if(pending.userID == userID)
            return;
    }

    PendingTransfer pending;
    pending.userID = userID;
    pending.sourceServerID = sourceServerID;
    pending.targetServerID = pWorld->serverID;
    pending.worldDatabaseID = pWorld->databaseID;
    pending.worldInstanceID = pWorld->instanceID;
    pending.worldName = pWorld->worldName;

    pWorld->waitingPlayers.push_back(pending);

    PlayerSession* pSession = GetPlayerManager()->GetSessionByID(userID);
    if(pSession)
    {
        pSession->serverID = sourceServerID;
        pSession->worldInstanceID = pWorld->instanceID;

    }
}

void WorldManager::FailPlayerJoin(ServerInfo* pServer, uint32 userID, const string& message)
{
    if(!pServer)
        return;

    ServerManager* pServerMgr = GetServerManager();
    if(!pServerMgr)
        return;

    bool found = false;
    for(auto& [_, world] : m_worldSessionsByInstance) 
    {
        if(found)
            break;

        auto& pending = world.waitingPlayers;
        for(auto& player : pending) 
        {
            if(player.userID == userID)
            {
                found = true;
                if(&player != &world.waitingPlayers.back())
                {
                    player = std::move(world.waitingPlayers.back());
                }

                world.waitingPlayers.pop_back();
                break;
            }
        }
    }

    pServerMgr->SendWorldPlayerFailPacket(pServer, userID, message);
}

WorldManager* GetWorldManager() { return WorldManager::GetInstance(); }