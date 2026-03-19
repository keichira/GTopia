#include "GamePlayer.h"
#include "../Server/MasterBroadway.h"
#include "../Context.h"
#include "IO/Log.h"
#include "Utils/Timer.h"
#include "Item/ItemInfoManager.h"
#include "Player/PlayerTribute.h"
#include "Player/RoleManager.h"
#include "Database/Table/PlayerDBTable.h"
#include "Player/PlayModManager.h"
#include "../World/WorldManager.h"
#include "Math/Random.h"
#include "Dialog/PlayerDialog.h"

GamePlayer::GamePlayer(ENetPeer* pPeer) 
: Player(pPeer), m_currentWorldID(0), m_joiningWorld(false), m_guestID(1), m_lastItemActivateTime(0), m_state(0)
{
}

GamePlayer::~GamePlayer()
{
}

void GamePlayer::OnHandleDatabase(QueryTaskResult&& result)
{
    if(HasState(PLAYER_STATE_LOGIN_GETTING_ACCOUNT)) {
        LoadingAccount(std::move(result));
    }

    result.Destroy();
}

void GamePlayer::OnHandleTCP(VariantVector&& result)
{
    if(HasState(PLAYER_STATE_CHECKING_SESSION)) {
        CheckingLoginSession(std::move(result));
    }

    if(!HasState(PLAYER_STATE_IN_GAME)) {
        return;
    }

    if(HasState(PLAYER_STATE_RENDERING_WORLD)) {
        HandleRenderWorld(std::move(result));
    }
}

void GamePlayer::StartLoginRequest(ParsedTextPacket<25>& packet)
{
    if(!HasState(PLAYER_STATE_LOGIN_REQUEST)) {
        SendLogonFailWithLog("");
        return;
    }

    RemoveState(PLAYER_STATE_LOGIN_REQUEST);

    if(!m_loginDetail.Serialize(packet, this, true)) {
        SendLogonFailWithLog("`4HUH?! ``Are you sure everything is alright?");
        return;
    }

    m_userID = m_loginDetail.user;
    SetState(PLAYER_STATE_CHECKING_SESSION);
    GetMasterBroadway()->SendCheckSessionPacket(GetNetID(), m_loginDetail.user, m_loginDetail.token, GetContext()->GetID());
}

void GamePlayer::CheckingLoginSession(VariantVector&& result)
{
    RemoveState(PLAYER_STATE_CHECKING_SESSION);
    /*if(!result[2].GetBool()) {
        SendLogonFailWithLog("`4OOPS! ``Please re-connect server says you're not belong to this server");
        return;
    }*/

    SetState(PLAYER_STATE_LOGIN_GETTING_ACCOUNT);

    QueryRequest req = MakeGetPlayerDataReq(m_userID, GetNetID());
    DatabasePlayerExec(GetContext()->GetDatabasePool(), DB_PLAYER_GET_DATA, req);
}

void GamePlayer::LoadingAccount(QueryTaskResult&& result)
{
    RemoveState(PLAYER_STATE_LOGIN_GETTING_ACCOUNT);
    if(!result.result) {
        SendLogonFailWithLog("`4OOPS! ``Failed to load your account, please re-connect");
        return;
    }

    uint32 roleID = result.result->GetField("RoleID", 0).GetUINT();
    if(roleID == 0) {
        roleID = 3; // default role
    }

    m_pRole = GetRoleManager()->GetRole(roleID);

    if(!m_pRole) {
        SendLogonFailWithLog("`4OOPS! ``Something went wrong while setting you up, please re-connect");
        LOGGER_LOG_WARN("Failed to set player role %d for user %d", roleID, m_userID);
        return;
    }

    m_inventory.SetVersion(m_loginDetail.protocol);
    string dbInv = result.result->GetField("Inventory", 0).GetString();
    if(!dbInv.empty()) {
        uint32 invMemEstimate = dbInv.size()/2;
        uint8* pInvData = new uint8[invMemEstimate];

        MemoryBuffer invMemBuffer(pInvData, invMemEstimate);
        m_inventory.Serialize(invMemBuffer, false, true);

        SAFE_DELETE_ARRAY(pInvData);
    }

    m_guestID = result.result->GetField("GuestID", 0).GetUINT();

    SetState(PLAYER_STATE_ENTERING_GAME);
    TransferingPlayerToGame();
}

void GamePlayer::TransferingPlayerToGame()
{
    string settings;
    settings += "proto=144"; /** search it what it effects in client */
    settings += "|server_tick=" + ToString(Time::GetSystemTime());
    settings += "|choosemusic=audio/mp3/about_theme.mp3";
    settings += "|usingStoreNavigation=1";
    settings += "|enableInventoryTab=1";

    ItemsClientData itemData = GetItemInfoManager()->GetClientData(m_loginDetail.platformType);
    auto pGameConfig = GetContext()->GetGameConfig();

    PlayerTributeClientData tributeData = GetPlayerTributeManager()->GetClientData();

    SendWelcomePacket(itemData.hash, pGameConfig->cdnServer, pGameConfig->cdnPath, settings, tributeData.hash);
}

void GamePlayer::HandleRenderWorld(VariantVector&& result)
{
    if(!HasState(PLAYER_STATE_RENDERING_WORLD)) {
        return;
    }

    if(result[1].GetINT() != TCP_RENDER_WORLD_FAIL) {
        SendOnConsoleMessage("`4System Message: `oYour world has been rendered");
    }
    else {
        SendOnConsoleMessage("`4System Message: `oFailed to render your world");
    }

    RemoveState(PLAYER_STATE_RENDERING_WORLD);
}

void GamePlayer::SaveToDatabase()
{
    uint32 invMemSize = m_inventory.GetMemEstimate(true);
    uint8* pInvData = new uint8[invMemSize];

    MemoryBuffer invMemBuffer(pInvData, invMemSize);
    m_inventory.Serialize(invMemBuffer, true, true);

    QueryRequest req = MakeSavePlayerReq(
        m_userID,
        m_pRole->GetID(),
        ToHex(pInvData, invMemSize),
        GetNetID()
    );

    DatabasePlayerExec(GetContext()->GetDatabasePool(), DB_PLAYER_SAVE, req);
    SAFE_DELETE_ARRAY(pInvData);
}

void GamePlayer::Update()
{
    UpdatePlayMods();

    if(m_currentWorldID != 0) {
        World* pWorld = GetWorldManager()->GetWorldByID(m_currentWorldID);
        if(pWorld) {
            if(m_characterData.NeededCharStateUpdate()) {
                pWorld->SendSetCharPacketToAll(this);
                m_characterData.SetNeedCharStateUpdate(false);
            }
        }
    }
}

string GamePlayer::GetDisplayName()
{
    string displayName;
    if(m_pRole->GetNameColor() != 0) {
        displayName += "`"; 
        displayName += m_pRole->GetNameColor();
    }
    displayName += m_pRole->GetPrefix();

    if(!m_loginDetail.tankIDName.empty()) {
        displayName += m_loginDetail.tankIDName;
    }
    else {
        displayName += m_loginDetail.requestedName + "_" + ToString(m_guestID);
    }

    displayName += m_pRole->GetSuffix();
    return displayName;
}

string GamePlayer::GetRawName()
{
    return m_loginDetail.tankIDName.empty() ? 
        m_loginDetail.requestedName + "_" + ToString(m_guestID)
        : m_loginDetail.tankIDName;
}

string GamePlayer::GetSpawnData(bool local)
{
    string spawnData;
    spawnData += "spawn|avatar\n";
    spawnData += "netID|" + ToString(GetNetID()) + "\n";
    spawnData += "userID|" + ToString(m_userID) + "\n";
    spawnData += "colrect|0|0|20|30\n"; //its ok to hardcoded (for now?)
    spawnData += "posXY|" + ToString(m_worldPos.x)  + "|" + ToString(m_worldPos.y) + "\n"; 
    spawnData += "name|" + GetDisplayName() + "``\n";
    spawnData += "country|" + m_loginDetail.country + "\n";
    spawnData += "invis|0\n"; // todo
    spawnData += "mstate|";
    spawnData += m_pRole->HasPerm(ROLE_PERM_MSTATE) ? "1\n" : "0\n";
    spawnData += "smstate|" ;
    spawnData += m_pRole->HasPerm(ROLE_PERM_SMSTATE) ? "1\n" : "0\n";
    spawnData += "onlineID|\n";

    if(local) {
        spawnData += "type|local\n";
    }

    return spawnData;
}

void GamePlayer::ToggleCloth(uint16 itemID)
{
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if(!pItem || pItem->bodyPart > BODY_PART_SIZE) {
        return;
    }

    if((pItem->type != ITEM_TYPE_CLOTHES && pItem->type != ITEM_TYPE_ARTIFACT) && !m_pRole->HasPerm(ROLE_PERM_CAN_WEAR_ANY)) {
        return;
    }

    if(pItem->type == ITEM_TYPE_ARTIFACT) {
        /**
         * 
         */
        return;
    }

    uint16 wornItem = m_inventory.GetClothes()[pItem->bodyPart];
    if(wornItem == pItem->id) {
        m_inventory.SetClothByPart(ITEM_ID_BLANK, pItem->bodyPart);

        if(pItem->playModType != PLAYMOD_TYPE_NONE) {
            RemovePlayMod(pItem->playModType);
        }

        PlayerInventory& playerInv = GetInventory();

        uint8 itemCount = playerInv.GetCountOfItem(pItem->id);
        switch(pItem->id) {
            case ITEM_ID_DIAMOND_HORN: {
                ModifyInventoryItem(ITEM_ID_DIAMOND_HORN, -itemCount);
                ModifyInventoryItem(ITEM_ID_DIAMOND_HORNS, itemCount);
                break;
            }
            case ITEM_ID_DIAMOND_HORNS: {
                ModifyInventoryItem(ITEM_ID_DIAMOND_HORNS, -itemCount);
                ModifyInventoryItem(ITEM_ID_DIAMOND_DEVIL_HORNS, itemCount);
                break;
            }
            case ITEM_ID_DIAMOND_DEVIL_HORNS: {
                ModifyInventoryItem(ITEM_ID_DIAMOND_DEVIL_HORNS, -itemCount);
                ModifyInventoryItem(ITEM_ID_DIAMOND_HORN, itemCount);
                break;
            }
        }
    }
    else {
        m_inventory.SetClothByPart(pItem->id, pItem->bodyPart);

        ItemInfo* pWornItem = GetItemInfoManager()->GetItemByID(wornItem);
        if(pWornItem) {
            RemovePlayMod(pWornItem->playModType);
        }

        if(pItem->playModType != PLAYMOD_TYPE_NONE) {
            AddPlayMod(pItem->playModType);
        }
    }

    if(m_currentWorldID != 0) {
        World* pWorld = GetWorldManager()->GetWorldByID(m_currentWorldID);
        pWorld->SendClothUpdateToAll(this);
    }
}

void GamePlayer::ModifyInventoryItem(uint16 itemID, int16 amount)
{
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if(!pItem || amount == 0) {
        return;
    }

    if(IsIllegalItem(itemID) && !m_pRole->HasPerm(ROLE_PERM_BYPASS_ILLEGAl_ITEM)) {
        return;
    }

    if(pItem->HasFlag(ITEM_FLAG_MOD) && !m_pRole->HasPerm(ROLE_PERM_USE_ITEM_TYPE_MOD)) {
        return;
    }

    if(amount < 0) {
        m_inventory.RemoveItem(itemID, -amount, this);

        if(m_inventory.GetCountOfItem(itemID) == 0 && m_inventory.IsWearingItem(itemID)) {
            ToggleCloth(itemID);
        }
    }
    else {
        m_inventory.AddItem(itemID, amount, this);
    }
}

void GamePlayer::TrashItem(uint16 itemID, uint8 amount)
{
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if(!pItem) {
        LOGGER_LOG_WARN("Player %d tried to trash non exist item %d ?!?!", m_userID, itemID)
        return;
    }

    if(amount > pItem->maxCanHold) {
        return;
    }

    if(amount > m_inventory.GetCountOfItem(itemID)) {
        PlaySFX("cant_place_tile.wav");
        return;
    }

    ModifyInventoryItem(itemID, -amount);

    PlaySFX("trash.vaw");
    SendOnConsoleMessage("Trashed " + ToString(amount) + " " + pItem->name);
}

void GamePlayer::AddPlayMod(ePlayModType modType)
{
    if(modType == PLAYMOD_TYPE_NONE) {
        return;
    }

    PlayMod* pPlayMod = m_characterData.AddPlayMod(modType);
    if(!pPlayMod) {
        return;
    }

    if(pPlayMod->GetTime() != 0) {
        SendOnConsoleMessage("`o" + pPlayMod->GetName() + " (`$" + pPlayMod->GetAddMessage() + " `omod added, `$" + Time::ConvertTimeToStr(pPlayMod->GetTime() * 1000) + "`oleft)");
    }
    else {
        SendOnConsoleMessage("`o" + pPlayMod->GetName() + " (`$" + pPlayMod->GetAddMessage() + " `omod added)");
    }

    UpdateNeededPlayModThings();
}

void GamePlayer::RemovePlayMod(ePlayModType modType)
{
    if(modType == PLAYMOD_TYPE_NONE) {
        return;
    }

    PlayMod* pPlayMod = m_characterData.RemovePlayMod(modType);
    if(!pPlayMod) {
        return;
    }

    SendOnConsoleMessage("`o" + pPlayMod->GetName() + " (`$" + pPlayMod->GetRemoveMessage() + " `omod removed)");
    UpdateNeededPlayModThings();
}

void GamePlayer::UpdateNeededPlayModThings()
{
    if(
        m_characterData.NeededSkinUpdate() && m_currentWorldID != 0
    ) {
        World* pWorld = GetWorldManager()->GetWorldByID(m_currentWorldID);
        if(!pWorld) {
            return;
        }

        if(m_characterData.NeededSkinUpdate()) {
            pWorld->SendSkinColorUpdateToAll(this);
            m_characterData.SetNeedSkinUpdate(false);
        }
    }
}

void GamePlayer::UpdatePlayMods()
{
    auto& reqUpdtMods = m_characterData.GetReqUpdatePlayMods();

    for(int32 i = reqUpdtMods.size() - 1; i >= 0; --i) {
        PlayerPlayModInfo& playMod = reqUpdtMods[i];
        
        if(playMod.modType == PLAYMOD_TYPE_CARRYING_A_TORCH) {
            if(m_currentWorldID == 0) {
                continue;
            }

            if(GetInventory().GetClothByPart(BODY_PART_HAND) != ITEM_ID_HAND_TORCH) {
                RemovePlayMod(PLAYMOD_TYPE_CARRYING_A_TORCH);
                continue;
            }

            UpdateTorchPlayMod();
        }
        else if(reqUpdtMods[i].timer.GetElapsedTime() >= playMod.durationMS) {
            RemovePlayMod(playMod.modType);
        }
    }
}

void GamePlayer::UpdateTorchPlayMod()
{
    if(RandomRangeInt(0, 250) != 18) { // is it a good idea? lol since tick based
        return;
    }

    GetInventory().RemoveItem(ITEM_ID_HAND_TORCH, 1, this);

    uint8 leftTorchCount = GetInventory().GetCountOfItem(ITEM_ID_HAND_TORCH);
    if(leftTorchCount == 0) {
        SendOnTalkBubble("`2My torch went out!", true);
        ToggleCloth(ITEM_ID_HAND_TORCH);
        return;
    }

    SendOnTalkBubble("`2My torch went out, i have " + ToString(leftTorchCount) + " more!", true);
}

void GamePlayer::SendPositionToWorldPlayers()
{
    if(m_currentWorldID == 0) {
        return;
    }

    World* pWorld = GetWorldManager()->GetWorldByID(m_currentWorldID);
    if(!pWorld) {
        return;
    }

    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_STATE;
    packet.posX = m_worldPos.x;
    packet.posY = m_worldPos.y;
    packet.netID = GetNetID();

    if(m_characterData.HasPlayerFlag(PLAYER_FLAG_FACING_LEFT)) {
        packet.SetFlag(NET_GAME_PACKET_FLAGS_FACINGLEFT);
    }

    pWorld->SendGamePacketToAll(&packet, this);
}