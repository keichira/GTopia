#include "WorldManager.h"
#include "../Context.h"
#include "../Server/MasterBroadway.h"
#include "../Server/GameServer.h"
#include "Database/Table/WorldDBTable.h"
#include "../Player/PlayerManager.h"
#include "IO/File.h"

#include "../Event/UDP/GamePacket/ItemActivateRequest.h"
#include "../Event/UDP/GamePacket/TileChangeRequest.h"
#include "../Event/UDP/GamePacket/State.h"
#include "../Event/UDP/GamePacket/ObjectActivateRequest.h"

WorldManager::WorldManager()
{
    RegisterEvents();
}

WorldManager::~WorldManager()
{
    Kill();
}

void WorldManager::Kill()
{
    for(auto& [_, pWorld] : m_worlds)
    {
        SAFE_DELETE(pWorld);
    }

    m_worlds.clear();
    m_worldNameCache.clear();
}

void WorldManager::HandleWorldInit(VariantVector&& result)
{
    if(result.size() < 3)
        return;

    string worldName = result[1].GetString();
    uint32 instanceID = result[2].GetUINT();
    uint32 databaseID = result[3].GetUINT();

    World* pWorld = new World();
    pWorld->SetInstanceID(instanceID);
    pWorld->SetDatabaseID(databaseID);
    pWorld->SetName(worldName);
    pWorld->SetState(WORLD_STATE_LOADING);

    AddWorld(pWorld);
    m_worldNameCache[ToLower(worldName)] = instanceID;
    StartWorldLoad(pWorld);
}

void WorldManager::StartWorldLoad(World* pWorld)
{
    if(!pWorld)
        return;

    if(!pWorld->InitWorld())
    {
        pWorld->GenerateWorld(WORLD_GENERATION_DEFAULT);
        pWorld->SetState(WORLD_STATE_READY);
        pWorld->SaveToDatabase();
    }
    else
    {
        pWorld->SetState(WORLD_STATE_READY);
    }

    GetMasterBroadway()->SendWorldInitResult(true, pWorld->GetInstanceID());
}

void WorldManager::HandlePlayerJoin(VariantVector&& result)
{
    if(result.size() < 7)
        return;

    int32 oprResult = result[1].GetINT();
    uint32 playerUserID = result[2].GetUINT();

    if(oprResult != TCP_RESULT_OK)
    {
        GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByUserID(playerUserID);
        if(pPlayer)
        {
            pPlayer->SendOnFailedToEnterWorld();
            pPlayer->SendOnConsoleMessage("Unable to join to this world, please try again later.");
        }
        return;
    }

    uint32 serverID = result[3].GetUINT();
    uint32 worldID = result[4].GetUINT();

    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByUserID(playerUserID);
    if(!pPlayer)
        return;

    if(serverID != GetContext()->GetID())
    {
        string serverIP = result[5].GetString();
        uint16 serverPort  = (uint16)result[6].GetUINT();

        PlayerLoginDetail& loginDetail = pPlayer->GetLoginDetail();
        loginDetail.loginMode = LOGON_MODE_TRANSFER;

        pPlayer->SendOnSendToServer(
            serverPort,
            loginDetail.token,
            pPlayer->GetUserID(),
            serverIP,
            loginDetail.loginMode
        );

        pPlayer->LogOff(false, false, false);
        return;
    }

    World* pWorld = GetWorldByInstanceID(worldID);
    if(!pWorld)
    {
        pPlayer->SendOnFailedToEnterWorld();
        pPlayer->SendOnConsoleMessage("Umm somehow you got lost between servers, try again.");
        return;
    }

    if(pWorld->GetState() == WORLD_STATE_DELETE)
    {
        pPlayer->SendOnFailedToEnterWorld();
        pPlayer->SendOnConsoleMessage("Unable to move you to this world, please try again in a few seconds.");
        return;
    }

    pWorld->AddPlayer(pPlayer, true);
}

void WorldManager::PlayerJoinRequest(GamePlayer* pPlayer, const string& worldName)
{
    if(!pPlayer || pPlayer->IsJoiningWorld())
        return;

    string targetWorldName = ToUpper(worldName);
    RemoveGTColorCodes(targetWorldName);

    if(targetWorldName.empty() || targetWorldName.size() > 18)
    {
        pPlayer->SendOnConsoleMessage("Sorry, world name length must be between 1 and 18.");
        pPlayer->SendOnFailedToEnterWorld();
        return;
    }

    if(!IsValidWorldName(targetWorldName))
    {
        pPlayer->SendOnConsoleMessage("Sorry, spaces and special characters are not allowed in world or door names. Try again.");
        pPlayer->SendOnFailedToEnterWorld();
        return;
    }

    if(targetWorldName == "EXIT")
    {
        pPlayer->SendOnConsoleMessage("Exit from what? Click back if you're done playing.");
        pPlayer->SendOnFailedToEnterWorld();
        return;
    }

    World* pWorld = GetWorldByName(targetWorldName);
    if(!pWorld)
    {
        GetMasterBroadway()->SendPlayerWorldJoin(pPlayer->GetUserID(), targetWorldName);
        return;
    }

    if(pWorld->GetState() == WORLD_STATE_DELETE)
    {
        pPlayer->SendOnFailedToEnterWorld();
        pPlayer->SendOnConsoleMessage("Unable to move you to this world, please try again in a few seconds.");
        return;
    }

    if(pWorld->GetState() == WORLD_STATE_LOADING)
    {
        return;
    }

    if(pWorld->GetPlayerCount() >= GetContext()->GetGameConfig()->worldMaxPlayerCount)
    {
        pPlayer->SendOnConsoleMessage(
            "Oops, `5" + targetWorldName + "`` already has `4" + ToString(GetContext()->GetGameConfig()->worldMaxPlayerCount) + "`` people in it. Try again later."
        );
        pPlayer->SendOnFailedToEnterWorld();
        return;
    }

    OnPlayerJoinRequest(pPlayer, pWorld);
}

void WorldManager::UpdateWorlds()
{
    if(m_lastWorldUpdateTime.GetElapsedTime() < GAME_TICK_MS)
        return;

    std::vector<uint32> deleteList;
    deleteList.reserve(m_worlds.size());

    for (auto& [worldID, pWorld] : m_worlds)
    {
        if(!pWorld)
            continue;

        if(pWorld->GetState() != WORLD_STATE_DELETE)
        {
            pWorld->Update();
        }

        if(pWorld->GetState() == WORLD_STATE_DELETE)
        {
            deleteList.push_back(worldID);
            continue;
        }
    }

    for (uint32 worldID : deleteList)
    {
        auto it = m_worlds.find(worldID);
        if (it == m_worlds.end())
            continue;

        if (it->second)
            SAFE_DELETE(it->second);

        m_worldNameCache.erase(ToLower(it->second->GetWorlName()));
        m_worlds.erase(it);
    }

    UpdatePendingLoadWorlds();
    m_lastWorldUpdateTime.Reset();
}

void WorldManager::UpdatePendingLoadWorlds()
{
    if (m_pendingLoad.empty())
        return;

    bool loadedOne = false;

    while (!loadedOne && !m_pendingLoad.empty())
    {
        World* pWorld = m_pendingLoad.front();
        m_pendingLoad.pop();

        if(!pWorld)
            continue;

        loadedOne = true;

        if(!pWorld->InitWorld())
        {
            pWorld->GenerateWorld(WORLD_GENERATION_DEFAULT);
            pWorld->SaveToDatabase();
        }
    }
}

World* WorldManager::GetWorldByName(const string& worldName)
{
    auto it = m_worldNameCache.find(worldName);
    if(it == m_worldNameCache.end())
        return nullptr;

    return GetWorldByInstanceID(it->second);
}

World* WorldManager::GetWorldByInstanceID(uint32 instanceID)
{
    if(instanceID == 0)
        return nullptr;

    auto it = m_worlds.find(instanceID);
    if(it != m_worlds.end()) 
        return it->second;

    return nullptr;
}

void WorldManager::AddWorld(World* pWorld)
{
    if(!pWorld)
        return;

    uint32 instanceID = pWorld->GetInstanceID();

    auto it = m_worlds.find(instanceID);
    if(it != m_worlds.end())
    {
        if(it->second && it->second != pWorld)
            SAFE_DELETE(it->second);

        m_worlds.erase(it);
    }

    m_worlds.insert_or_assign(instanceID, pWorld);
    m_worldNameCache[ToUpper(pWorld->GetWorlName())] = instanceID;
}

void WorldManager::OnPlayerJoinRequest(GamePlayer* pPlayer, World* pWorld)
{
    if(!pPlayer || !pWorld)
        return;

    if(pWorld->GetState() != WORLD_STATE_READY)
    {
        return;
    }

    pWorld->AddPlayer(pPlayer, true);
}

void WorldManager::OnHandleGamePacket(ENetEvent& event)
{
    GameUpdatePacket* pGamePacket = GetGamePacketFromEnetPacket(event.packet);
    if(!pGamePacket) {
        return;
    }

    GamePlayer* pPlayer = (GamePlayer*)event.peer->data;
    if(event.peer != pPlayer->GetPeer()) {
        return;
    }

    if(!pPlayer->HasState(PLAYER_STATE_IN_GAME)) {
        return;
    }

    World* pWorld = GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    if(pGamePacket->type != NET_GAME_PACKET_NPC && pGamePacket->type != NET_GAME_PACKET_PING_REPLY 
        && pGamePacket->type != NET_GAME_PACKET_PING_REQUEST && pGamePacket->type != NET_GAME_PACKET_SET_ICON_STATE
    ) {
        pPlayer->GetLastActionTime().Reset();
    }

    m_packetEvents.Dispatch((eGamePacketType)pGamePacket->type, pPlayer, pWorld, pGamePacket);
}

void WorldManager::SaveAllToDatabase()
{
    for(auto& [_, pWorld] : m_worlds)
    {
        if(!pWorld)
            continue;

        pWorld->SaveToDatabase();
    }
}

void WorldManager::RegisterEvents()
{
    RegisterPacketEvent<ItemActivateRequest>(NET_GAME_PACKET_ITEM_ACTIVATE_REQUEST);
    RegisterPacketEvent<TileChangeRequest>(NET_GAME_PACKET_TILE_CHANGE_REQUEST);
    RegisterPacketEvent<State>(NET_GAME_PACKET_STATE);
    RegisterPacketEvent<ObjectActivateRequest>(NET_GAME_PACKET_ITEM_ACTIVATE_OBJECT_REQUEST);
}

WorldManager* GetWorldManager() { return WorldManager::GetInstance(); }

