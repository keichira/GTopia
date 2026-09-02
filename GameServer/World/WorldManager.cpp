#include "WorldManager.h"
#include "../Context.h"
#include "../Player/PlayerManager.h"
#include "../Server/GamePresenceManager.h"
#include "../Server/GameServer.h"
#include "../Server/MasterBroadway.h"
#include "Database/Table/WorldDBTable.h"
#include "IO/File.h"
#include "Math/Math.h"

#include "../Event/UDP/GamePacket/ItemActivateRequest.h"
#include "../Event/UDP/GamePacket/ObjectActivateRequest.h"
#include "../Event/UDP/GamePacket/State.h"
#include "../Event/UDP/GamePacket/TileActivateRequest.h"
#include "../Event/UDP/GamePacket/TileChangeRequest.h"

WorldManager::WorldManager()
{
    RegisterEvents();
}

WorldManager::~WorldManager()
{
    Kill();
}

void WorldManager::HandleWorldInit(TCPPacketReader& reader)
{
    string worldName;
    uint32 instanceID = 0;
    uint32 databaseID = 0;

    if (!reader.ReadString(worldName) || !reader.Read<uint32>(instanceID) || !reader.Read<uint32>(databaseID))
        return;

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
    if (!pWorld)
        return;

    if (!pWorld->InitWorld())
    {
        pWorld->GenerateWorld(WORLD_GENERATION_DEFAULT);
        pWorld->SaveToDatabase();
    }

    auto presenceUserIDs = pWorld->GetRequiredPresenceUserIDs();
    if (presenceUserIDs.empty())
    {
        pWorld->SetState(WORLD_STATE_READY);
        pWorld->UpdatePresenceNeededThings(false);
        GetMasterBroadway()->SendWorldInitResult(true, pWorld->GetInstanceID());
    }
    else
    {
        pWorld->SetState(WORLD_STATE_LOADING);

        GetGamePresenceManager()->RequestPresenceForWorld(pWorld->GetInstanceID(), presenceUserIDs);
        LOGGER_LOG_DEBUG("World %s is waiting for %d presence snapshots before going READY.",
                         pWorld->GetWorlName().c_str(), (int32)presenceUserIDs.size());
    }
}

void WorldManager::Kill()
{
    for (auto& [_, pWorld] : m_worlds)
    {
        SAFE_DELETE(pWorld);
    }

    m_worlds.clear();
    m_worldNameCache.clear();
}

void WorldManager::HandlePlayerJoin(TCPPacketReader& reader)
{
    int32 oprResult = 0;
    uint32 playerUserID = 0;

    if (!reader.Read<int32>(oprResult) || !reader.Read<uint32>(playerUserID))
        return;

    if (oprResult != TCP_RESULT_OK)
    {
        GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByUserID(playerUserID);
        if (pPlayer)
        {
            pPlayer->SendOnFailedToEnterWorld();
            pPlayer->SendOnConsoleMessage("Unable to join to this world, please try again later.");
        }
        return;
    }

    uint32 serverID = 0;
    uint32 worldID = 0;
    string doorID;
    string serverIP;
    uint32 serverPort = 0;

    if (!reader.Read<uint32>(serverID) || !reader.Read<uint32>(worldID) || !reader.ReadString(doorID) ||
        !reader.ReadString(serverIP) || !reader.Read<uint32>(serverPort))
    {
        return;
    }

    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByUserID(playerUserID);
    if (!pPlayer)
        return;

    pPlayer->GetLoginDetail().doorID = doorID;

    if (serverID != GetContext()->GetID())
    {
        PlayerLoginDetail& loginDetail = pPlayer->GetLoginDetail();
        loginDetail.loginMode = LOGON_MODE_TRANSFER;

        pPlayer->SendOnSendToServer((uint16)serverPort, loginDetail.token, pPlayer->GetUserID(), serverIP,
                                    loginDetail.loginMode, loginDetail.doorID);

        pPlayer->LogOff(true, false, false, false);
        return;
    }

    World* pWorld = GetWorldByInstanceID(worldID);
    if (!pWorld)
    {
        pPlayer->SendOnFailedToEnterWorld();
        pPlayer->SendOnConsoleMessage("Umm somehow you got lost between servers, try again.");
        return;
    }

    if (pWorld->GetState() == WORLD_STATE_DELETE)
    {
        pPlayer->SendOnFailedToEnterWorld();
        pPlayer->SendOnConsoleMessage("Unable to move you to this world, please try again in a few seconds.");
        return;
    }

    OnPlayerJoinRequest(pPlayer, pWorld);
}

void WorldManager::PlayerJoinRequest(GamePlayer* pPlayer, const string& worldName)
{
    if (!pPlayer || pPlayer->IsJoiningWorld() || worldName.empty())
        return;

    if (!gPacketPool.IsHugeSlotAvailable())
    {
        pPlayer->SendOnFailedToEnterWorld();
        pPlayer->SendOnConsoleMessage("Sorry, map traffic is heavy, please retry in a second.");
        return;
    }

    string targetWorldName = ToUpper(worldName);
    RemoveGTColorCodes(targetWorldName);

    auto targetWorld = Split(targetWorldName, '|');
    if (targetWorld.empty())
        return;

    targetWorldName = targetWorld[0];
    StripWhiteSpace(targetWorldName);

    if (targetWorldName.empty() || targetWorldName.size() > 18)
    {
        pPlayer->SendOnConsoleMessage("Sorry, world name length must be between 1 and 18.");
        pPlayer->SendOnFailedToEnterWorld();
        return;
    }

    if (!IsValidWorldName(targetWorldName))
    {
        pPlayer->SendOnConsoleMessage(
            "Sorry, spaces and special characters are not allowed in world or door names. Try again.");
        pPlayer->SendOnFailedToEnterWorld();
        return;
    }

    /*if (pPlayer->GetCurrentWorld() == 0 && (targetWorldName.find('_') != string::npos || targetWorldName.find(' ') !=
    string::npos))
    {
        pPlayer->SendOnConsoleMessage("Sorry, spaces and special characters are not allowed in world or door names. Try
    again."); pPlayer->SendOnFailedToEnterWorld();
    }*/

    if (targetWorldName == "EXIT")
    {
        if (pPlayer->GetCurrentWorld() == 0)
        {
            pPlayer->SendOnConsoleMessage("Exit from what? Click back if you're done playing.");
            pPlayer->SendOnFailedToEnterWorld();
        }
        else
        {
            World* pPlayerWorld = GetWorldByInstanceID(pPlayer->GetCurrentWorld());
            if (!pPlayerWorld)
                return;

            pPlayerWorld->PlayerLeaveWorld(pPlayer, true);
            GetWorldManager()->SendWorldMenuRequest(pPlayer);
        }
        return;
    }

    string doorID = (targetWorld.size() > 1 ? targetWorld[1] : "");
    StripWhiteSpace(doorID);

    World* pWorld = GetWorldByName(targetWorldName);
    if (!pWorld)
    {
        GetMasterBroadway()->SendPlayerWorldJoin(pPlayer->GetUserID(), targetWorldName, doorID);
        return;
    }
    else if (pPlayer->GetCurrentWorld() == pWorld->GetInstanceID())
    {
        Vector2Float vDoorPos = pWorld->GetTileManager()->GetMapStartWorldPos(doorID);
        pPlayer->SendEnterDoorPacket(vDoorPos);
        return;
    }

    if (pWorld->GetState() == WORLD_STATE_DELETE)
    {
        pPlayer->SendOnFailedToEnterWorld();
        pPlayer->SendOnConsoleMessage("Unable to move you to this world, please try again in a few seconds.");
        return;
    }

    if (pWorld->GetState() == WORLD_STATE_LOADING)
        return;

    if (pWorld->GetBannedPlayers().IsBanned(pPlayer->GetAddressNum()))
    {
        pPlayer->SendOnConsoleMessage("`4Oh no!`` You've been banned from that world by its owner!  Try again later "
                                      "after the world ban wears off.");
        pPlayer->SendOnFailedToEnterWorld();
        return;
    }

    if (pWorld->GetPlayerCount() >= GetContext()->GetGameConfig()->worldMaxPlayerCount)
    {
        pPlayer->SendOnConsoleMessage("Oops, `5" + targetWorldName + "`` already has `4" +
                                      ToString(GetContext()->GetGameConfig()->worldMaxPlayerCount) +
                                      "`` people in it. Try again later.");
        pPlayer->SendOnFailedToEnterWorld();
        return;
    }

    OnPlayerJoinRequest(pPlayer, pWorld);
}

void WorldManager::UpdateWorlds()
{
    if (m_lastWorldUpdateTime.GetElapsedTime() < GAME_TICK_MS)
        return;

    std::vector<uint32> deleteList;
    deleteList.reserve(m_worlds.size());

    for (auto& [worldID, pWorld] : m_worlds)
    {
        if (!pWorld)
            continue;

        if (pWorld->GetState() != WORLD_STATE_DELETE)
        {
            pWorld->Update();
        }

        if (pWorld->GetState() == WORLD_STATE_DELETE)
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
        {
            it->second->OnWorldDelete();
            SAFE_DELETE(it->second);
        }

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

        if (!pWorld)
            continue;

        loadedOne = true;

        if (!pWorld->InitWorld())
        {
            pWorld->GenerateWorld(WORLD_GENERATION_DEFAULT);
            pWorld->SaveToDatabase();
        }
    }
}

void WorldManager::OnWorldPresenceReady(uint32 worldID)
{
    World* pWorld = GetWorldByInstanceID(worldID);
    if (!pWorld)
        return;

    pWorld->SetState(WORLD_STATE_READY);
    pWorld->UpdatePresenceNeededThings(false);
    GetMasterBroadway()->SendWorldInitResult(true, worldID);
}

World* WorldManager::GetWorldByName(const string& worldName)
{
    auto it = m_worldNameCache.find(worldName);
    if (it == m_worldNameCache.end())
        return nullptr;

    return GetWorldByInstanceID(it->second);
}

World* WorldManager::GetWorldByDatabaseID(uint32 databaseID)
{
    for (auto& [_, pWorld] : m_worlds)
    {
        if (!pWorld)
            continue;

        if (pWorld->GetDatabaseID() == databaseID)
            return pWorld;
    }

    return nullptr;
}

World* WorldManager::GetWorldByInstanceID(uint32 instanceID)
{
    if (instanceID == 0)
        return nullptr;

    auto it = m_worlds.find(instanceID);
    if (it != m_worlds.end())
        return it->second;

    return nullptr;
}

void WorldManager::AddWorld(World* pWorld)
{
    if (!pWorld)
        return;

    uint32 instanceID = pWorld->GetInstanceID();

    auto it = m_worlds.find(instanceID);
    if (it != m_worlds.end())
    {
        if (it->second && it->second != pWorld)
            SAFE_DELETE(it->second);

        m_worlds.erase(it);
    }

    m_worlds.insert_or_assign(instanceID, pWorld);
    m_worldNameCache[ToUpper(pWorld->GetWorlName())] = instanceID;
}

void WorldManager::OnPlayerJoinRequest(GamePlayer* pPlayer, World* pWorld)
{
    if (!pPlayer || !pWorld)
        return;

    if (pWorld->GetState() != WORLD_STATE_READY)
    {
        pPlayer->SendOnFailedToEnterWorld();
        return;
    }

    World* pPlayerWorld = GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (pPlayerWorld)
    {
        if (pPlayerWorld == pWorld)
            return;

        pPlayerWorld->PlayerLeaveWorld(pPlayer, true);
    }

    pWorld->AddPlayer(pPlayer, true);
}

void WorldManager::OnHandleGamePacket(NetworkEvent& event)
{
    if (!event.pPacket)
        return;

    GameUpdatePacket* pGamePacket = GetGamePacketFromEnetPacket(event.pPacket->payload, event.pPacket->dataLength);
    if (!pGamePacket)
    {
        gPacketPool.Release(event.pPacket);
        return;
    }

    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(event.netID);
    if (!pPlayer || !pPlayer->HasState(PLAYER_STATE_IN_GAME))
    {
        gPacketPool.Release(event.pPacket);
        return;
    }

    World* pWorld = GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
    {
        gPacketPool.Release(event.pPacket);
        return;
    }

    CRASH_SET("LastGamePacket", pGamePacket->type);

    if (pGamePacket->type != NET_GAME_PACKET_NPC && pGamePacket->type != NET_GAME_PACKET_PING_REPLY &&
        pGamePacket->type != NET_GAME_PACKET_PING_REQUEST && pGamePacket->type != NET_GAME_PACKET_SET_ICON_STATE)
    {
        pPlayer->GetLastActionTime().Reset();
    }

    if (pGamePacket->type == NET_GAME_PACKET_SET_ICON_STATE)
    {
        pPlayer->SetIconState(pGamePacket->field_11);
        pWorld->SendGamePacketToAll(pGamePacket, pPlayer);
        gPacketPool.Release(event.pPacket);
        return;
    }

    m_packetEvents.Dispatch((eGamePacketType)pGamePacket->type, pPlayer, pWorld, pGamePacket);
    gPacketPool.Release(event.pPacket);
}

void WorldManager::SaveAllToDatabase()
{
    if (!GetContext()->GetDatabasePool()->GetWorker(0)->IsConnected())
        return;

    for (auto& [_, pWorld] : m_worlds)
    {
        if (!pWorld)
            continue;

        pWorld->SaveToDatabase();
    }
}

void WorldManager::SendWorldMenuRequest(GamePlayer* pPlayer)
{
    if (!pPlayer)
        return;

    GamePresenceManager* pPresenceMgr = GetGamePresenceManager();

    if (m_worldListCacheTimer.GetElapsedTime() >= 15000 || m_cachedPopularWorldList.empty())
    {
        m_cachedPopularWorldList.clear();

        pPresenceMgr->SortWorldsByPlayerCount();

        uint32 totalWorlds = pPresenceMgr->GetWorldPresenceCount();
        int32 popularTargetCount = Min(4, (int32)totalWorlds);

        for (int32 i = 0; i < popularTargetCount; ++i)
        {
            auto* pPresence = pPresenceMgr->GetWorldPresenceDataByIndex(i);
            if (!pPresence || pPresence->isSignalJammed)
                continue;

            WorldListElement item;
            item.name = pPresence->name;
            item.playerCount = pPresence->playerCount;

            m_cachedPopularWorldList.push_back(item);
        }

        int32 remainingWorlds = totalWorlds - popularTargetCount;
        int32 randomTargetCount = Min(3, remainingWorlds);

        for (int32 i = 0; i < randomTargetCount; ++i)
        {
            auto* pPresence = pPresenceMgr->GetRandomWorldPresenceData(popularTargetCount);
            if (!pPresence || pPresence->isSignalJammed)
                continue;

            WorldListElement item;
            item.name = pPresence->name;
            item.playerCount = pPresence->playerCount;
            m_cachedPopularWorldList.push_back(item);
        }

        m_worldListCacheTimer.Reset();
    }

    DialogBuilder db;

    if (pPlayer->GetLoginDetail().protocol >= 94) // might be wrong
        db.AddHeading("Top Worlds");

    if (m_cachedPopularWorldList.empty())
    {
        db.AddFloater("START", 0, 0.5f, 3529161471);
    }

    for (auto& world : m_cachedPopularWorldList)
    {
        db.AddFloater(world.name, world.playerCount, 0.5f, 3529161471);
    }

    pPlayer->SendOnRequestWorldSelectMenu(db.Get());
}

void WorldManager::RegisterEvents()
{
    RegisterPacketEvent<ItemActivateRequest>(NET_GAME_PACKET_ITEM_ACTIVATE_REQUEST);
    RegisterPacketEvent<TileChangeRequest>(NET_GAME_PACKET_TILE_CHANGE_REQUEST);
    RegisterPacketEvent<State>(NET_GAME_PACKET_STATE);
    RegisterPacketEvent<ObjectActivateRequest>(NET_GAME_PACKET_ITEM_ACTIVATE_OBJECT_REQUEST);
    RegisterPacketEvent<TileActivateRequest>(NET_GAME_PACKET_TILE_ACTIVATE_REQUEST);
}

WorldManager* GetWorldManager()
{
    return WorldManager::GetInstance();
}
