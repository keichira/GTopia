#include "GamePlayer.h"
#include "../Context.h"
#include "../Server/MasterBroadway.h"
#include "../World/WorldManager.h"
#include "Database/Table/PlayerDBTable.h"
#include "Dialog/DropItemDialog.h"
#include "Dialog/PlayerDialog.h"
#include "Dialog/RegisterDialog.h"
#include "Dialog/RenderWorldDialog.h"
#include "IO/Log.h"
#include "Item/ItemInfoManager.h"
#include "Math/Math.h"
#include "Math/Random.h"
#include "Player/PlayModManager.h"
#include "Player/PlayerTribute.h"
#include "Player/RoleManager.h"
#include "PlayerManager.h"
#include "Proton/ProtonUtils.h"
#include "Utils/GrowUtils.h"
#include "Utils/Timer.h"

GamePlayer::GamePlayer()
    : m_currentWorldID(0), m_joiningWorld(false), m_guestID(0), m_lastItemActivateTime(0), m_state(0), m_flags(0),
      m_gems(0), m_progressData(this), m_modController(this), m_activeBattlePetSlot(0), m_lockAccessTileIndex(-1),
      m_lockAccessOwnerID(-1), m_tradeMgr(this), m_extraData(this)
{
    RandomizeNextDBSaveTime();
}

GamePlayer::~GamePlayer()
{
    ClosePaginatedDialog();
}

void GamePlayer::Update()
{
    m_modController.Update();

    if (m_currentWorldID == 0 && (m_characterData.needCharStateUpdate || m_characterData.needSkinUpdate))
    {
        m_characterData.needCharStateUpdate = false;
        m_characterData.needSkinUpdate = false;
    }

    if (m_currentWorldID != 0)
    {
        if (World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID))
        {
            if (m_characterData.needCharStateUpdate)
            {
                pWorld->SendSetCharPacketToAll(this);
                m_characterData.needCharStateUpdate = false;
            }

            if (m_characterData.needSkinUpdate)
            {
                pWorld->SendSkinColorUpdateToAll(this);
                m_characterData.needSkinUpdate = false;
            }
        }
    }

    if ((HasState(PLAYER_STATE_LOGIN_REQUEST) || HasState(PLAYER_STATE_ENTERING_GAME)) &&
        m_logonStartTime.GetElapsedTime() >= 180000)
    {
        LogOff(true, false, true);
        return;
    }

    if (HasState(PLAYER_STATE_IN_GAME))
    {
        if (!m_targetJoinWorld.empty())
        {
            GetWorldManager()->PlayerJoinRequest(this, m_targetJoinWorld + "|" + m_loginDetail.doorID);
            m_targetJoinWorld = "";
            m_loginDetail.doorID = "";
        }
    }
}

void GamePlayer::StartLoginRequest(ParsedTextPacket<40>& packet)
{
    if (!HasState(PLAYER_STATE_LOGIN_REQUEST))
        return;

    m_logonStartTime.Reset();

    if (!m_loginDetail.Serialize(packet, this, true))
    {
        SendLogonFailWithLog("`4HUH?! ``Are you sure everything is alright?");
        LogOff(true, false, true);
        return;
    }

    m_userID = m_loginDetail.user;
    GetMasterBroadway()->SendCheckSessionPacket(GetNetID(), m_loginDetail.user, m_loginDetail.token,
                                                GetContext()->GetID());

    CRASH_SET("LastPlayerUserID", m_userID);
}

void GamePlayer::HandleCheckSession(VariantVector&& result)
{
    bool foundSession = result[2].GetBool();
    if (!foundSession)
    {
        SendLogonFailWithLog("`4OOPS! ``Please re-connect server says you're not belong to this server");
        return;
    }

    if (GamePlayer* pTarget = GetPlayerManager()->IsPlayerAlreadyOn(this))
    {
        SendOnConsoleMessage("`4ALREADY ON?!`` : This account was already online, kicking it off so you can log on.");
        pTarget->SendOnConsoleMessage(
            "`4This account is being activated from another device, kicking you off so they can get on");
        pTarget->LogOff(true, true, false);
    }

    if (m_loginDetail.loginMode == LOGON_MODE_TRANSFER)
    {
        m_currentWorldID = result[3].GetUINT();
    }

    TransferToGame();
}

void GamePlayer::TransferToGame()
{
    string settings;
    settings += "proto=144";
    settings += "|server_tick=" + ToString(Time::GetSystemTime());
    settings += "|choosemusic=audio/mp3/about_theme.mp3";
    settings += "|usingStoreNavigation=1";
    settings += "|enableInventoryTab=1";

    auto itemData = GetItemInfoManager()->GetClientData(m_loginDetail.platformType, m_loginDetail.gameVersion);
    if (!itemData)
    {
        SendLogonFailWithLog("`4Oops`o, something went wrong please re-login.");
        LOGGER_LOG_ERROR("Failed to get client data platform: %d, gameversion: %f", m_loginDetail.platformType,
                         m_loginDetail.gameVersion);
        return;
    }

    GameConfig* pGameConfig = GetContext()->GetGameConfig();
    RemoveState(PLAYER_STATE_LOGIN_REQUEST);
    SetState(PLAYER_STATE_ENTERING_GAME);

    SendWelcomePacket(itemData->hash, pGameConfig->cdnServer, pGameConfig->cdnPath, settings, 0);
}

void GamePlayer::SaveToDatabase()
{
    if (!GetContext()->GetDatabasePool()->GetWorker(0)->IsConnected())
        return;

    uint32 invMemSize = m_inventory.GetMemEstimate(true);
    MemoryBuffer invMemBuffer(invMemSize);
    m_inventory.Serialize(invMemBuffer, true, true);

    uint32 progressMemSize = m_progressData.GetMemEstimate();
    MemoryBuffer progressMemBuffer(progressMemSize);
    m_progressData.Serialize(progressMemBuffer, true);

    uint32 extraMemSize = m_extraData.GetMemEstimate();
    MemoryBuffer extraMemBuffer(extraMemSize);
    m_extraData.Serialize(extraMemBuffer, true);

    uint32 worldID = 0;
    if (m_currentWorldID != 0)
    {
        if (World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID))
        {
            worldID = pWorld->GetDatabaseID();
        }
    }

    uint32 roleID = GetRoleManager()->GetDefaultRoleID();
    if (m_pRole)
        roleID = m_pRole->GetID();
    else
        LOGGER_LOG_WARN("Player %s (%d) SaveDB role is NULL setting to default role %d", GetRawName(), GetUserID(),
                        roleID);

    QueryRequest req =
        PlayerDB::Save(m_userID, roleID, string((char*)invMemBuffer.GetData(), invMemBuffer.GetOffset()), 0, m_flags,
                       worldID, string((char*)progressMemBuffer.GetData(), progressMemBuffer.GetOffset()), m_gems,
                       string((char*)extraMemBuffer.GetData(), extraMemBuffer.GetOffset()), GetNetID());

    DatabasePlayerExec(GetContext()->GetDatabasePool(), req);
}

void GamePlayer::BuildForBulkDatabaseSave(VariantVector& outParams)
{
    uint32 invSize = m_inventory.GetMemEstimate(true);
    string invStr;
    invStr.resize(invSize);

    MemoryBuffer invMemBuffer(invStr.data(), invSize);
    m_inventory.Serialize(invMemBuffer, true, true);

    uint32 progressSize = m_progressData.GetMemEstimate();
    string progressStr;
    progressStr.resize(progressSize);

    MemoryBuffer progressMemBuffer(progressStr.data(), progressSize);
    m_progressData.Serialize(progressMemBuffer, true);

    uint32 extraSize = m_extraData.GetMemEstimate();
    string extraStr;
    extraStr.resize(extraSize);

    MemoryBuffer extraMemBuffer(extraStr.data(), extraSize);
    m_extraData.Serialize(extraMemBuffer, true);

    uint32 worldID = 0;
    if (m_currentWorldID != 0)
    {
        if (World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID))
            worldID = pWorld->GetDatabaseID();
    }

    uint32 roleID = m_pRole ? m_pRole->GetID() : GetRoleManager()->GetDefaultRoleID();

    outParams.emplace_back(roleID);

    Variant invVar;
    invVar.SetBinary(std::move(invStr));
    outParams.emplace_back(std::move(invVar));

    outParams.emplace_back((uint32)0);
    outParams.emplace_back(m_flags);
    outParams.emplace_back(worldID);

    Variant progressVar;
    progressVar.SetBinary(std::move(progressStr));
    outParams.emplace_back(std::move(progressVar));

    outParams.emplace_back(m_gems);

    Variant extraVar;
    extraVar.SetBinary(std::move(extraStr));
    outParams.emplace_back(std::move(extraVar));

    outParams.emplace_back(m_userID);
}

void GamePlayer::LogOff(bool forceDelete, bool saveToDb, bool endSession, bool sendNetworkPackets)
{
    if (forceDelete)
    {
        SetState(PLAYER_STATE_DELETE);
    }

    if (!HasState(PLAYER_STATE_LOGGING_OFF))
    {
        SetState(PLAYER_STATE_LOGGING_OFF);

        if (HasState(PLAYER_STATE_IN_GAME) && saveToDb)
        {
            SaveToDatabase();
        }

        if (forceDelete && m_currentWorldID != 0 && HasState(PLAYER_STATE_IN_GAME))
        {
            if (World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID))
            {
                pWorld->PlayerLeaveWorld(this, true);
            }
        }

        if (forceDelete && sendNetworkPackets)
        {
            SendUDPPacket(GetNetID(), NET_MESSAGE_GAME_MESSAGE, "action|logoff\n");
            SendUDPDisconnectPacket(GetNetID());
        }

        SetState(PLAYER_STATE_DELETE);

        if (endSession)
        {
            GetMasterBroadway()->SendEndPlayerSession(m_userID);
        }
    }
}

void GamePlayer::CheckLimitsForAccountCreation(bool fromDialog, const VariantVector& extraData)
{
    QueryRequest req;

    if (m_loginDetail.platformType == Proton::PLATFORM_ID_WINDOWS && !m_loginDetail.sid.empty())
    {
        req = PlayerDB::CountBySidMacIP(m_loginDetail.sid, m_loginDetail.mac, GetAddress(), GetNetID());
    }
    else if (m_loginDetail.platformType == Proton::PLATFORM_ID_ANDROID && !m_loginDetail.gid.empty())
    {
        req = PlayerDB::CountByGidMacIP(m_loginDetail.gid, m_loginDetail.mac, GetAddress(), GetNetID());
    }
    else if (m_loginDetail.platformType == Proton::PLATFORM_ID_IOS && !m_loginDetail.vid.empty())
    {
        req = PlayerDB::CountByVidMacIP(m_loginDetail.vid, m_loginDetail.mac, GetAddress(), GetNetID());
    }
    else
    {
        req = PlayerDB::CountByMacIP(m_loginDetail.mac, GetAddress(), GetNetID());
    }

    req.AddExtraData(fromDialog);
    if (!extraData.empty())
    {
        // name, pass, verify pass
        req.AddExtraData(extraData[0], extraData[1], extraData[2]);
    }

    req.callback = &GamePlayer::CheckAccountCreationLimitCB;
    DatabasePlayerExec(GetContext()->GetDatabasePool(), req);
}

void GamePlayer::OnEnterGameCheckAndSendToWorldIfPossibleCB(QueryTaskResult&& result)
{
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(result.ownerID);
    if (!pPlayer)
        return;

    pPlayer->GetLoginDetail().loginMode = LOGON_MODE_TRANSFER;
    if (!result.result)
    {
        GetWorldManager()->SendWorldMenuRequest(pPlayer);
        return;
    }

    string worldName = result.result->GetField("Name", 0).GetString();
    pPlayer->RemoveState(PLAYER_STATE_ENTERING_GAME);
    pPlayer->SetState(PLAYER_STATE_IN_GAME);
    pPlayer->SetTargetJoinWorld(worldName, pPlayer->GetLoginDetail().doorID);
}

void GamePlayer::OnEnterGameCB(QueryTaskResult&& result)
{
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(result.ownerID);
    if (!pPlayer)
        return;

    if (!result.result)
    {
        pPlayer->SendLogonFailWithLog("`4OOPS! ``Something went wrong please re-connect");
        return;
    }

    PlayerLoginDetail& loginDetail = pPlayer->GetLoginDetail();
    string growIDName = result.result->GetField("Name", 0).GetString();
    if (growIDName.empty())
    {
        loginDetail.requestedName = result.result->GetField("GuestName", 0).GetString();
        loginDetail.tankIDName = "";
        loginDetail.tankIDPass = "";
    }
    else
    {
        loginDetail.tankIDName = growIDName;
    }

    uint32 roleID = result.result->GetField("RoleID", 0).GetINT();
    if (roleID == 0)
    {
        roleID = GetRoleManager()->GetDefaultRoleID();
    }

    Role* pRole = GetRoleManager()->GetRole(roleID);
    if (!pRole)
    {
        pPlayer->SendLogonFailWithLog("`4OOPS! ``Something went wrong while setting you up, please re-connect");
        LOGGER_LOG_WARN("Failed to set player role %d for user %d", roleID, pPlayer->GetUserID());
        pPlayer->LogOff(true, false, true);
        return;
    }
    pPlayer->SetRole(pRole);

    uint32 skinColor = result.result->GetField("SkinColor", 0).GetUINT();
    if (skinColor != 0)
    {
        pPlayer->GetCharData().SetSkinColor(skinColor);
    }

    pPlayer->SetFlags(result.result->GetField("Flags", 0).GetINT());
    pPlayer->SetGems(result.result->GetField("Gems", 0).GetINT());

    PlayerInventory& inventory = pPlayer->GetInventory();

    inventory.SetVersion(pPlayer->GetLoginDetail().protocol);
    string dbInv = result.result->GetField("Inventory", 0).GetString();

    if (!dbInv.empty())
    {
        MemoryBuffer invMemBuffer(dbInv.data(), dbInv.size());
        inventory.Serialize(invMemBuffer, false, true);
    }

    string progressData = result.result->GetField("ProgressData", 0).GetString();
    if (!progressData.empty())
    {
        MemoryBuffer progressMemBuffer(progressData.data(), progressData.size());
        pPlayer->GetProgressData().Serialize(progressMemBuffer, false);
    }

    string extraData = result.result->GetField("ExtraData", 0).GetString();
    if (!extraData.empty())
    {
        MemoryBuffer extraMemBuffer(extraData.data(), extraData.size());
        pPlayer->GetExtraData().Serialize(extraMemBuffer, false);
    }

    if (inventory.GetCountOfItem(ITEM_ID_FIST) == 0)
    {
        inventory.AddItem(ITEM_ID_FIST, 1);
    }

    if (inventory.GetCountOfItem(ITEM_ID_WRENCH) == 0)
    {
        inventory.AddItem(ITEM_ID_WRENCH, 1);
    }

    pPlayer->SetGuestID(result.result->GetField("GuestID", 0).GetINT());

    /*for(uint8 i = 0; i < BODY_PART_SIZE; ++i)
    {
        uint16 cloth = inventory.GetClothByPart((eBodyPart)i);

        ItemInfo* pItem = GetItemInfoManager()->GetItemByID(cloth);
        if(!pItem) {
            continue;
        }

        if(pItem->type == ITEM_TYPE_CLOTHES && pItem->playModType != PLAYMOD_TYPE_NONE) {
            pPlayer->AddPlayMod(pItem->playModType, true);
        }
    }*/

    if (loginDetail.loginMode == LOGON_MODE_WELCOME)
    {
        pPlayer->SendOnConsoleMessage("Welcome back, `w" + pPlayer->GetDisplayName(false) + "`o.");
    }

    pPlayer->SendGems(true);

    pPlayer->SetSearchName(ToLower(pPlayer->GetRawName()));
    pPlayer->SendSetHasGrowID(pPlayer->HasGrowID() ? true : false);

    pPlayer->SendInventoryPacket();

    if (loginDetail.gameVersion >= 5.47)
    {
        for (int32 i = 0; i < FEATURE_FLAG_COUNT; ++i)
        {
            if (i != FEATURE_FLAG_HOME_WORLD_TUTORIAL && i != FEATURE_FLAG_WRENCH_TUTORIAL)
            {
                pPlayer->EnableFeature((eClientFeatureFlag)(i));
            }
        }

        pPlayer->SendOnSetFeatureEnableFlags();
    }

    uint32 worldID = result.result->GetField("LastWorld", 0).GetINT();

    if (worldID != 0 && loginDetail.loginMode == LOGON_MODE_WELCOME)
    {
        QueryRequest req = WorldDB::GetByID(worldID, pPlayer->GetNetID());
        req.callback = &OnEnterGameCheckAndSendToWorldIfPossibleCB;
        DatabaseWorldExec(GetContext()->GetDatabasePool(), req);
        return;
    }

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (pWorld)
    {
        pPlayer->RemoveState(PLAYER_STATE_ENTERING_GAME);
        pPlayer->SetState(PLAYER_STATE_IN_GAME);

        pPlayer->SetTargetJoinWorld(pWorld->GetWorlName(), pPlayer->GetLoginDetail().doorID);
        pPlayer->SetCurrentWorld(0);
    }
    else
    {
        pPlayer->RemoveState(PLAYER_STATE_ENTERING_GAME);
        pPlayer->SetState(PLAYER_STATE_IN_GAME);
        GetWorldManager()->SendWorldMenuRequest(pPlayer);
        pPlayer->SetCurrentWorld(0);
    }
}

void GamePlayer::CheckAccountCreationLimitCB(QueryTaskResult&& result)
{
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(result.ownerID);
    if (!pPlayer || !result.result)
        return;

    Variant* pMac = result.result->GetFieldSafe("mac_count", 0);
    Variant* pIP = result.result->GetFieldSafe("ip_count", 0);
    Variant* pOther = nullptr;
    bool shouldSetOther = true;

    PlayerLoginDetail& loginDetail = pPlayer->GetLoginDetail();

    if (loginDetail.platformType == Proton::PLATFORM_ID_WINDOWS && !loginDetail.sid.empty())
    {
        pOther = result.result->GetFieldSafe("sid_count", 0);
    }
    else if (loginDetail.platformType == Proton::PLATFORM_ID_ANDROID && !loginDetail.gid.empty())
    {
        pOther = result.result->GetFieldSafe("gid_count", 0);
    }
    else if (loginDetail.platformType == Proton::PLATFORM_ID_IOS && !loginDetail.vid.empty())
    {
        pOther = result.result->GetFieldSafe("vid_count", 0);
    }
    else
    {
        shouldSetOther = false;
    }

    if (!pMac && !pIP && (!shouldSetOther || !pOther))
    {
        pPlayer->SendOnTalkBubble("`4Oops! ``Something went wrong while checking for account creating.", true);
        return;
    }

    GameConfig* pGameConfig = GetContext()->GetGameConfig();
    if (pIP->GetUINT() > pGameConfig->maxAccountsPerIP || pMac->GetUINT() > pGameConfig->maxAccountsPerMac ||
        (shouldSetOther &&
         (pOther->GetUINT() > (loginDetail.platformType == Proton::PLATFORM_ID_WINDOWS ? pGameConfig->maxAccountsPerSid
                               : loginDetail.platformType == Proton::PLATFORM_ID_ANDROID
                                   ? pGameConfig->maxAccountsPerGid
                                   : pGameConfig->maxAccountsPerVid))))
    {
        pPlayer->SendOnTalkBubble(
            "`4Oops! ``You've reached the max `5GrowID ``accounts you can make for this device or IP address!", true);
        return;
    }

    bool fromDialog = result.extraData[0].GetBool();
    if (fromDialog)
    {
        QueryRequest req = PlayerDB::GrowIDExists(result.extraData[1].GetString(), pPlayer->GetNetID());
        req.extraData = result.extraData;

        req.callback = &GamePlayer::AccountCreationNameExistsCB;
        DatabasePlayerExec(GetContext()->GetDatabasePool(), req);
    }
    else
    {
        RegisterDialog::Request(pPlayer);
    }
}

void GamePlayer::AccountCreationNameExistsCB(QueryTaskResult&& result)
{
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(result.ownerID);
    if (!pPlayer || !result.result)
        return;

    if (result.result->GetRowCount() > 0)
    {
        RegisterDialog::Request(pPlayer, result.extraData[1].GetString(), result.extraData[2].GetString(),
                                result.extraData[3].GetString(),
                                "`4Oops!`` The name `w" + result.extraData[1].GetString() +
                                    "`` is so cool someone else has already taken it. Please choose a different name.");
    }
    else
    {
        QueryRequest req = PlayerDB::GrowIDCreate(pPlayer->GetUserID(), result.extraData[1].GetString(),
                                                  result.extraData[2].GetString(), pPlayer->GetNetID());
        req.AddExtraData(result.extraData[1].GetString(), result.extraData[2].GetString());

        req.callback = &GamePlayer::CreateAccountFinalCB;
        DatabasePlayerExec(GetContext()->GetDatabasePool(), req);
    }
}

void GamePlayer::CreateAccountFinalCB(QueryTaskResult&& result)
{
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(result.ownerID);
    if (!pPlayer)
        return;

    pPlayer->GetLoginDetail().tankIDName = result.extraData[0].GetString();
    pPlayer->GetLoginDetail().tankIDPass = result.extraData[1].GetString();

    RegisterDialog::Success(pPlayer, result.extraData[0].GetString(), result.extraData[1].GetString());
}

void GamePlayer::SetTargetJoinWorld(const string& worldName, const string& doorID)
{
    m_targetJoinWorld = worldName;
    m_loginDetail.doorID = doorID;
}

void GamePlayer::SendEnterDoorPacket(Vector2Float doorWorldPos)
{
    m_worldPos = doorWorldPos;

    SendOnZoomCamera(10000.0f);
    SendOnSetFreezeState(PLAYER_FREEZE_STATE_NONE, 0);

    if (m_currentWorldID != 0)
    {
        if (World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID))
        {
            pWorld->SendPositionCorrectionToAll(this, doorWorldPos);
            pWorld->SendPlayPositionedToAll(this, "door_open.wav");
        }
    }
}

void GamePlayer::HandleRenderWorld(VariantVector&& result)
{
    if (!HasState(PLAYER_STATE_RENDERING_WORLD))
        return;

    int32 renderResult = result[2].GetINT();

    if (renderResult == TCP_RESULT_OK)
    {
        World* pWorld = GetWorldManager()->GetWorldByDatabaseID(result[4].GetUINT());
        if (!pWorld)
        {
            SendOnConsoleMessage("`oYour world \"`4<UNKNOWN>`o\" has been rendered!");
            RenderWorldDialog::OnRendered(this, "`4<UNKNOWN>");
        }
        else
        {
            SendOnConsoleMessage("`oYour world \"`#" + pWorld->GetWorlName() + "`o\" has been rendered!");
            RenderWorldDialog::OnRendered(this, pWorld->GetWorlName());
        }
    }
    else
    {
        SendOnConsoleMessage("`4OOPS! ``Unable to render your world right now.");
    }

    RemoveState(PLAYER_STATE_RENDERING_WORLD);
}

void GamePlayer::SendPositionToWorldPlayers()
{
    if (m_currentWorldID == 0)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
    if (!pWorld)
        return;

    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_STATE;
    packet.field_8.x = m_worldPos.x;
    packet.field_8.y = m_worldPos.y;
    packet.field_4 = GetNetID();

    if (m_characterData.HasCharFlag(CHARACTER_FLAG_FACING_LEFT))
    {
        packet.SetFlag(GAME_PACKET_FLAG_FACING_LEFT);
    }

    pWorld->SendGamePacketToAll(&packet, this);
}

Vector2Float GamePlayer::GetWorldPosCenter()
{
    return Vector2Float(m_worldPos.x + 16, m_worldPos.y + 16);
}

RectFloat GamePlayer::GetPlayerWorldRect()
{
    return RectFloat(m_worldPos.x, m_worldPos.y, m_worldPos.x + 20, m_worldPos.y + 30);
}

string GamePlayer::GetDisplayName(bool checkWorld)
{
    string displayName;
    if (m_pRole->GetNameColor() != 0)
    {
        displayName += "`";
        displayName += m_pRole->GetNameColor();
    }

    if (checkWorld && m_currentWorldID != 0)
    {
        if (World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID))
        {
            if (pWorld->IsPlayerWorldOwner(this))
                displayName += "`2";
            else if (pWorld->IsPlayerWorldAdmin(this))
                displayName += "`^";
        }
    }

    if (m_progressData.IsTitleActive(PLAYER_TITLE_DOCTOR))
    {
        displayName += "`4Dr. ";
    }

    displayName += m_pRole->GetPrefix() + GetRawName() + m_pRole->GetSuffix();

    if (m_progressData.IsTitleActive(PLAYER_TITLE_LEGEND))
    {
        displayName += " of Legend";
    }
    return displayName;
}

string GamePlayer::GetRawName()
{
    return m_loginDetail.tankIDName.empty() ? m_loginDetail.requestedName + "_" + ToString(m_guestID)
                                            : m_loginDetail.tankIDName;
}

string GamePlayer::GetSpawnData(bool local)
{
    string spawnData;
    spawnData += "spawn|avatar\n";
    spawnData += "netID|" + ToString(GetNetID()) + "\n";
    spawnData += "userID|" + ToString(m_userID) + "\n";
    spawnData += "colrect|0|0|20|30\n";
    spawnData += "posXY|" + ToString(m_worldPos.x) + "|" + ToString(m_worldPos.y) + "\n";
    spawnData += "name|" + GetDisplayName(true) + "``\n";
    spawnData += "country|" + GetCountryData() + "\n";
    spawnData += "invis|0\n";
    spawnData += "mstate|" + string(m_pRole->HasPerm("state.mod"_hash) ? "1\n" : "0\n");
    spawnData += "smstate|" + string(m_pRole->HasPerm("state.smod"_hash) ? "1\n" : "0\n");
    spawnData += "onlineID|\n";

    if (local)
        spawnData += "type|local\n";

    return spawnData;
}

string GamePlayer::GetCountryData()
{
    string out = m_loginDetail.country;

    if (m_progressData.IsTitleActive(PLAYER_TITLE_MAX_LVL))
        out += "|maxLevel";

    if (m_progressData.IsTitleActive(PLAYER_TITLE_DOCTOR))
        out += "|doctor";

    if (m_progressData.IsTitleActive(PLAYER_TITLE_MASTER))
        out += "|master";

    if (m_progressData.IsTitleActive(PLAYER_TITLE_G4G))
        out += "|g4g";

    return out;
}

void GamePlayer::ToggleCloth(int32 itemID)
{
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem || pItem->bodyPart > BODY_PART_SIZE)
        return;

    if (pItem->type != ITEM_TYPE_CLOTHES && pItem->type != ITEM_TYPE_ARTIFACT)
        return;

    if (pItem->type == ITEM_TYPE_ARTIFACT)
        /**
         *
         */
        return;

    int16 wornItem = m_inventory.GetClothByPart((eBodyPart)pItem->bodyPart);
    if (wornItem == pItem->id)
    {
        m_inventory.SetClothByPart(ITEM_ID_BLANK, pItem->bodyPart);

        if (pItem->playModType != PLAYMOD_TYPE_NONE)
        {
            m_modController.RemovePlayMod(pItem->playModType);
        }

        PlayerInventory& playerInv = GetInventory();

        uint8 itemCount = playerInv.GetCountOfItem(pItem->id);
        switch (pItem->id)
        {
            case ITEM_ID_DIAMOND_HORN:
            {
                ModifyInventoryItem(ITEM_ID_DIAMOND_HORN, -itemCount);
                ModifyInventoryItem(ITEM_ID_DIAMOND_HORNS, itemCount);
                break;
            }
            case ITEM_ID_DIAMOND_HORNS:
            {
                ModifyInventoryItem(ITEM_ID_DIAMOND_HORNS, -itemCount);
                ModifyInventoryItem(ITEM_ID_DIAMOND_DEVIL_HORNS, itemCount);
                break;
            }
            case ITEM_ID_DIAMOND_DEVIL_HORNS:
            {
                ModifyInventoryItem(ITEM_ID_DIAMOND_DEVIL_HORNS, -itemCount);
                ModifyInventoryItem(ITEM_ID_DIAMOND_HORN, itemCount);
                break;
            }

            case ITEM_ID_BATTLE_LEASH:
            {
                ToggleBattlePetLeash(false);
                break;
            }
        }
    }
    else
    {
        m_inventory.SetClothByPart(pItem->id, pItem->bodyPart);

        ItemInfo* pWornItem = GetItemInfoManager()->GetItemByID(wornItem);
        if (pWornItem)
        {
            m_modController.RemovePlayMod(pWornItem->playModType);
        }

        if (pItem->playModType != PLAYMOD_TYPE_NONE)
        {
            m_modController.AddPlayMod(pItem->playModType);
        }
    }

    if (m_currentWorldID != 0)
    {
        World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
        pWorld->SendClothUpdateToAll(this);
    }
}

void GamePlayer::ToggleBattlePetLeash(bool forceFirstSlot)
{
    if (!forceFirstSlot)
    {
        m_activeBattlePetSlot = 1 - m_activeBattlePetSlot;
    }

    if (m_activeBattlePetSlot == 1)
    {
        if (m_progressData.GetProgress(PLAYER_PROGRESS_PET_2_0) == 0)
        {
            m_activeBattlePetSlot = 0;
        }
    }

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
    if (!pWorld)
        return;

    int32 petID = 0;
    if (m_activeBattlePetSlot == 0)
    {
        petID = m_progressData.GetProgress(PLAYER_PROGRESS_PET_1_0);
    }
    else
    {
        petID = m_progressData.GetProgress(PLAYER_PROGRESS_PET_2_0);
    }

    pWorld->SendBattlePetPacketToAll(PET_EVENT_EQUIP, GetNetID(), petID);
}

bool GamePlayer::TryWearAllItemsFromDressup(TileInfo* pTile)
{
    if (!pTile)
        return false;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
    if (!pWorld)
        return false;

    TileExtra_Dressup* pTileExtra = pTile->GetExtra<TileExtra_Dressup>();
    if (!pTileExtra)
    {
        SendOnTalkBubble("Huh? The dressup is gone!", false);
        return false;
    }

    if (!pWorld->PlayerHasAccessOnTile(this, pTile))
    {
        SendOnTalkBubble("Not your phone booth!", false);
        return false;
    }

    std::vector<int32> fitItemVec;
    for (int32 i = 0; i < 9; ++i)
    {
        if (pTileExtra->clothes[i] == ITEM_ID_BLANK)
            continue;

        fitItemVec.push_back(1);
        fitItemVec.push_back(pTileExtra->clothes[i]);
    }

    if (!m_inventory.CanAllItemsFit(fitItemVec))
    {
        SendOnTalkBubble("I need more inventory space!", false);
        return false;
    }

    if (fitItemVec.empty())
        return true;

    for (int32 i = 0; i < 9; ++i)
    {
        if (pTileExtra->clothes[i] != ITEM_ID_BLANK)
        {
            ModifyInventoryItem(pTileExtra->clothes[i], 1);
        }

        pTileExtra->SetCloth(i, ITEM_ID_BLANK);
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (pItem)
    {
        SendOnTalkBubble("I emptied out my " + pItem->name + "!", false);
    }

    return true;
}

void GamePlayer::SendGems(bool skipAnim)
{
    SendOnSetBux(m_gems, skipAnim, HasFlag(PLAYER_FLAG_SUPPORTER), HasFlag(PLAYER_FLAG_SUPER_SUPPORTER));
}

void GamePlayer::ModifyGems(int32 count, bool sendToPlayer)
{
    if (count == 0)
        return;
    m_gems += count;

    if (sendToPlayer)
    {
        SendGems(false);
    }
}

void GamePlayer::GiveXP(uint32 amount)
{
    uint32 playerLevel = Sqrt((m_progressData.GetProgress(PLAYER_PROGRESS_XP) / 50) - 2);
    if (playerLevel == 125)
        return;

    m_progressData.AddProgress(PLAYER_PROGRESS_XP, amount);
    uint32 playerNewLevel = Sqrt((m_progressData.GetProgress(PLAYER_PROGRESS_XP) / 50) - 2);

    if (playerNewLevel > 125)
    {
        playerNewLevel = 125;
        m_progressData.SetProgress(PLAYER_PROGRESS_XP, 50 * (125 * 125 + 2));
    }

    if (playerNewLevel > playerLevel)
    {
        World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
        if (!pWorld)
        {
            SendOnConsoleMessage("You are now level " + ToString(playerNewLevel) + "!");
            return;
        }

        pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_LEVELUP, m_worldPos);
        pWorld->SendTalkBubbleAndConsoleToAll(
            GetDisplayName(false) + " `wis now level " + ToString(playerNewLevel) + "!", false, this);
    }
}

uint32 GamePlayer::GetPlayerLevel()
{
    uint32 playerXP = m_progressData.GetProgress(PLAYER_PROGRESS_XP);
    if (playerXP < 100)
        return 0;
    return Sqrt((playerXP / 50) - 2);
}

uint32 GamePlayer::GetPlayerNextLevelXP()
{
    uint32 currentLevel = GetPlayerLevel();
    return ((currentLevel) * (currentLevel) * 50) + 100;
}

uint32 GamePlayer::NormalizeSkinColor(uint32 skinColor)
{
    bool isBasicColor = (skinColor == 0x505C78FF) || (skinColor == 0x647296FF) || (skinColor == 0x788AB4FF) ||
                        (skinColor == 0x8295C3FF) || (skinColor == 0x96ACE1FF) || (skinColor == 0xAAC3FFFF) ||
                        (skinColor == 0xB4CEFFFF) || (skinColor == 0xC8E5FFFF);

    if (isBasicColor)
        return skinColor;

    if (HasFlag(PLAYER_FLAG_SUPPORTER) || HasFlag(PLAYER_FLAG_SUPER_SUPPORTER))
    {
        bool isSupporterColor = (skinColor == 0xA3DDB1FF) || (skinColor == 0xC5C241FF) || (skinColor == 0x2B4BD7FF) ||
                                (skinColor == 0x2A8A41FF) || (skinColor == 0xD551A9FF) || (skinColor == 0xF0F0F0FF);

        if (isSupporterColor)
            return skinColor;
    }

    if (HasFlag(PLAYER_FLAG_SUPER_SUPPORTER))
    {
        bool isSuperSupColor = (skinColor == 0xD27A3CFF) || (skinColor == 0xEFCBB1FF) || (skinColor == 0x0B94FFFF) ||
                               (skinColor == 0x5032FFA0);

        if (isSuperSupColor)
            return skinColor;
    }

    return 0x8295C3FF;
}

void GamePlayer::ModifyInventoryItem(int32 itemID, int16 amount)
{
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem || amount == 0)
        return;

    if (amount > 0 && IsIllegalItem(pItem->id) && !m_pRole->HasPerm("bypass.item_illegal"_hash))
        return;

    if (amount > 0 && pItem->HasFlag(ITEM_FLAG_MOD) && !m_pRole->HasPerm("bypass.item_mod"_hash))
        return;

    if (amount < 0)
    {
        m_inventory.RemoveItem(pItem->id, -amount, this);
        if (m_inventory.GetCountOfItem(pItem->id) == 0 && m_inventory.IsWearingItem(pItem->id))
        {
            ToggleCloth(pItem->id);
        }
    }
    else
    {
        m_inventory.AddItem(pItem->id, amount, this);
    }
}

void GamePlayer::TrashItem(int32 itemID, uint16 amount)
{
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
    {
        LOGGER_LOG_WARN("Player %d tried to trash non-existent item %d?!", m_userID, itemID);
        return;
    }

    if (amount > pItem->maxCanHold)
        return;
    if (amount > m_inventory.GetCountOfItem(pItem->id))
    {
        PlaySFX("cant_place_tile.wav");
        return;
    }

    ModifyInventoryItem(pItem->id, -amount);
    PlaySFX("trash.vaw");
    SendOnConsoleMessage("Trashed " + ToString(amount) + " " + pItem->name);
}

void GamePlayer::DropItem(int32 itemID, uint16 amount, bool openDialog)
{
    if (m_currentWorldID == 0)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
        return;

    InventoryItemInfo* pInvItem = m_inventory.GetItemByID(pItem->id);
    if (!pInvItem)
        return;

    if (amount > pInvItem->count)
        return;

    if (pItem->type == ITEM_TYPE_PETFISH && pInvItem->count != amount)
    {
        SendOnTalkBubble("Please don't chop up the fish", true);
        return;
    }

    if (pItem->maxCanHold == 0)
    {
        SendOnTalkBubble("You can't drop that.", true);
        PlaySFX("cant_place_tile.wav");
        return;
    }

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
    if (!pWorld)
        return;

    Vector2Float random = GetRandomPlayerItemDropOffset();

    Vector2Float playerCenter;
    playerCenter.x = m_worldPos.x + 20 * 0.5f;
    playerCenter.y = m_worldPos.y + 30 * 0.5f;

    float facing = m_characterData.HasCharFlag(CHARACTER_FLAG_FACING_LEFT) ? -0.75f : 0.75f;

    Vector2Float dropPos;
    dropPos.x = playerCenter.x + random.x + facing * 32.f;
    dropPos.y = playerCenter.y + random.y;

    TileInfo* pTile = pWorld->GetTileManager()->GetTileByWorldPos(dropPos.x, dropPos.y);
    if (!pTile || pTile->IsCollidable())
    {
        SendOnTalkBubble("You can't drop that here, face somewhere with open space.", true);
        PlaySFX("cant_place_tile.wav");
        return;
    }

    Vector2Float vTileWorldPos = pTile->GetWorldPos();
    dropPos.x = Clamp(dropPos.x, vTileWorldPos.x + 2.f, vTileWorldPos.x + 29.f);
    dropPos.y = Clamp(dropPos.y, vTileWorldPos.y + 2.f, vTileWorldPos.y + 29.f);

    if (pWorld->GetObjectManager()->GetCounfOfObjestsInRect(pTile->GetRect()) > 19)
    {
        SendOnTalkBubble("You can't drop that here, find an emptier spot!", true);
        PlaySFX("cant_place_tile.wav");
        return;
    }

    if (IsMainDoor(pTile->GetDisplayedItem()))
    {
        SendOnTalkBubble("You can't drop items on the white door.", true);
        PlaySFX("audio/cant_place_tile.wav");
        return;
    }

    if (openDialog)
    {
        DropItemDialog::Request(this, pInvItem);
        return;
    }

    ModifyInventoryItem(pItem->id, -amount);

    if (pTile->GetFG() == ITEM_ID_BUNNY_EGG && pItem->id == ITEM_ID_MAGIC_EGG)
    {
        TileExtra_MagicEgg* pTileExtra = pTile->GetExtra<TileExtra_MagicEgg>();
        if (!pTileExtra)
            return;

        uint32 eggCount = pTileExtra->eggCount;

        for (int32 i = amount; i > 0; --i)
        {
            if ((eggCount + 1) > 1000)
            {
                float burstChance = (float)(eggCount - 999) / 100000.0f;
                eggCount++;

                if (RandomRangeFloat(0, 1) < burstChance)
                {
                    pTileExtra->eggCount = eggCount;
                    SendOnTalkBubble("`4The oversized egg burst!``", false);
                    pWorld->OnBunnyEggBreak(this, pTile);
                    pTile->SetFG(ITEM_ID_BLANK, pWorld->GetTileManager());
                    break;
                }
            }
            else
            {
                eggCount++;
            }

            if (eggCount >= 2000)
            {
                pWorld->DropObjectOnTile(pTile, pItem->id, i, GetRandomPlayerItemDropOffset(), true);
                eggCount = 2000;
                break;
            }
        }

        if (pTile->GetFG() != ITEM_ID_BLANK && eggCount > 1000)
        {
            string warnMsg;

            if (pTileExtra->eggCount >= 1400 && pTileExtra->eggCount < 1601)
            {
                warnMsg = "`6This oversized egg has a good chance to burst!``";
            }
            else if (pTileExtra->eggCount >= 1601 && pTileExtra->eggCount < 1801)
            {
                warnMsg = "`8This oversized egg has a huge chance to burst!``";
            }
            else if (pTileExtra->eggCount >= 1801)
            {
                warnMsg = "`4This oversized egg is so big it could burst at any moment!``";
            }

            if (!warnMsg.empty())
                SendOnTalkBubble(warnMsg, false);
        }

        pTileExtra->eggCount = eggCount;

        pWorld->SendTileUpdate(pTile);
        pWorld->SendParticleEffectToAll((amount > 3) ? PARTICLE_EFFECT_DAISYPILE_BIG : PARTICLE_EFFECT_DAISYPILE,
                                        pTile->GetWorldPosCenter());
        return;
    }

    pWorld->DropObjectOnTile(pTile, pItem->id, amount, dropPos - pTile->GetWorldPosCenter(), true);
}

void GamePlayer::SendLockAccessRequest(GamePlayer* pOwner, TileInfo* pLockTile)
{
    if (!pOwner && !pLockTile && pOwner->HasState(PLAYER_STATE_DELETE))
        return;

    if (pOwner->GetLastSentAccessTime().GetElapsedTime() < 5000)
    {
        pOwner->SendOnTalkBubble("`4You need to wait a little bit before accessing someone else.``", false);
        return;
    }

    TileExtra_Lock* pTileExtra = pLockTile->GetExtra<TileExtra_Lock>();
    if (!pTileExtra)
        return;

    if (pTileExtra->HasAccess(GetUserID()))
    {
        pOwner->SendOnTalkBubble("That person already has access to this lock.", false);
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pLockTile->GetFG());
    if (!pItem || pItem->type != ITEM_TYPE_LOCK)
        return;

    m_lastSentAccessTime.Reset();
    m_lockAccessOwnerID = pOwner->GetNetID();
    SetLockAccessTile(pLockTile->GetMapIndex());

    string notifyMsg =
        pOwner->GetDisplayName(true) + "`w wants to add you to a " + pItem->name + "`w. Wrench yourself to accept.";
    SendOnTalkBubble(notifyMsg, false);
    SendOnConsoleMessage(notifyMsg);
    PlaySFX("secret.wav");

    pOwner->SendOnTalkBubble("Offered " + GetDisplayName(false) + "`w access to lock.", false);
}

void GamePlayer::SetLockAccessTile(int32 lockIndex)
{
    m_lockAccessTileIndex = lockIndex;
    if (lockIndex == -1)
        m_lockAccessOwnerID = -1;
}

TileInfo* GamePlayer::GetLockAcessTile()
{
    World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
    if (!pWorld)
        return nullptr;

    return pWorld->GetTileManager()->GetTile(m_lockAccessTileIndex);
}

void GamePlayer::AcceptLockAccess()
{
    if (m_lockAccessTileIndex == -1)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
    if (!pWorld)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(m_lockAccessTileIndex);
    if (!pTile)
        return;

    TileExtra_Lock* pTileExtra = pTile->GetExtra<TileExtra_Lock>();
    if (!pTileExtra)
        return;

    GamePlayer* pOwner = GetPlayerManager()->GetPlayerByNetID(m_lockAccessOwnerID);
    if (!pOwner || m_currentWorldID != pOwner->GetCurrentWorld())
    {
        SendOnTalkBubble("The lock owner has left!", false);
        SetLockAccessTile(-1);
        return;
    }

    if (pTileExtra->ownerID != pOwner->GetUserID())
    {
        SendOnTalkBubble("Sorry, the lock is gone!", false);
        SetLockAccessTile(-1);
        return;
    }

    if (pTileExtra->GetTotalAccessedCount() > 25)
    {
        SendOnTalkBubble("Sorry, that lock has the maximum players on it already!", false);
        SetLockAccessTile(-1);
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem || pItem->type != ITEM_TYPE_LOCK)
        return;

    SetLockAccessTile(-1);

    pTileExtra->accessList.push_back(GetUserID());
    pWorld->SendTileUpdate(pTile);

    pWorld->SendConsoleMessageToAll(GetDisplayName(false) + "`o was given access to a " + pItem->name + "`o.");
    PlaySFX("secret.wav");

    pWorld->SendNameChangeToAll(this);
}

TileInfo* GamePlayer::GetTilePlayerOn()
{
    if (m_currentWorldID == 0)
        return nullptr;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
    if (!pWorld)
        return nullptr;

    return pWorld->GetTileManager()->GetTileByWorldPos(m_worldPos);
}

TileInfo* GamePlayer::GetTilePlayerOnCenter()
{
    if (m_currentWorldID == 0)
        return nullptr;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
    if (!pWorld)
        return nullptr;

    Vector2Float worldPos = m_worldPos;
    m_worldPos.x += (m_characterData.avatarSize.x - 1.0f);
    m_worldPos.y += (m_characterData.avatarSize.y / 2.0f);

    return pWorld->GetTileManager()->GetTileByWorldPos(worldPos);
}

float GamePlayer::GetDistToTile(TileInfo* pGoalTile)
{
    if (!pGoalTile)
        return 0.0f;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
    if (!pWorld)
        return 0.0f;

    TileInfo* pPlayerTile = pWorld->GetTileManager()->GetTileByWorldPos(m_worldPos);
    if (!pPlayerTile)
        return 0.0f;

    Vector2Float vPlayerTilePos = pPlayerTile->GetWorldPos();
    Vector2Float vGoalPos = pGoalTile->GetWorldPos();

    return Sqrt((vPlayerTilePos.x - vGoalPos.x) * (vPlayerTilePos.x - vGoalPos.x) +
                (vPlayerTilePos.y - vGoalPos.y) * (vPlayerTilePos.y - vGoalPos.y));
}

uint32 GamePlayer::GetDistToTileInTiles(TileInfo* pGoalTile)
{
    return GetDistToTile(pGoalTile) / 32;
}

bool GamePlayer::HasLOSToTile(TileInfo* pGoalTile)
{
    if (!pGoalTile)
        return false;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
    if (!pWorld)
        return false;

    TileInfo* pPlayerTile = pWorld->GetTileManager()->GetTileByWorldPos(m_worldPos);
    if (!pPlayerTile)
        return false;

    Vector2Int& vPlayerTilePos = pPlayerTile->GetPos();
    Vector2Int& vGoalPos = pGoalTile->GetPos();

    int32 dx = vGoalPos.x - vPlayerTilePos.x;
    int32 dy = vGoalPos.y - vPlayerTilePos.y;

    int32 steps = (Abs(dx) + Abs(dy)) * 2;
    if (steps == 0)
        return true;

    float stepX = (float)dx / (float)(steps * 2);
    float stepY = (float)dy / (float)(steps * 2);

    const float offsets[4][2] = {{0.25f, 0.25f}, {0.75f, 0.25f}, {0.75f, 0.75f}, {0.25f, 0.75f}};

    WorldTileManager* pTileMgr = pWorld->GetTileManager();
    ItemInfoManager* pItemMgr = GetItemInfoManager();

    for (int32 sample = 0; sample < 4; ++sample)
    {
        float x = vPlayerTilePos.x + offsets[sample][0];
        float y = vPlayerTilePos.y + offsets[sample][1];

        int32 lastX = -1, lastY = -1;

        for (int32 i = 0; i < steps; ++i)
        {
            int32 tileX = (int32)x;
            int32 tileY = (int32)y;

            if (tileX != lastX || tileY != lastY)
            {
                lastX = tileX;
                lastY = tileY;

                if (tileX == vGoalPos.x && tileY == vGoalPos.y)
                    break;

                TileInfo* pTile = pTileMgr->GetTile(tileX, tileY);
                if (pTile)
                {
                    if (pTile->GetFG() == ITEM_ID_BEDROCK)
                        return false;

                    if (pWorld->IsTileCollidableForPlayer(this, pTile, false) &&
                        !pWorld->PlayerHasAccessOnTile(this, pTile))
                        return false;
                }
            }

            x += stepX;
            y += stepY;
        }
    }

    return true;
}

bool GamePlayer::AbleToWorldKickOrPullSomeone(GamePlayer* pTarget)
{

    if (!pTarget || m_currentWorldID == 0)
        return false;

    if (m_currentWorldID != pTarget->GetCurrentWorld())
        return false;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
    if (!pWorld)
        return false;

    TileInfo* pPlayerTile = GetTilePlayerOn();
    if (!pPlayerTile)
        return false;

    if (pPlayerTile->GetType() == ITEM_TYPE_LOCK)
        return false;

    TileInfo* pLockTile = pWorld->GetTileManager()->GetTileParentTileWithWorldLock(pPlayerTile);
    if (!pLockTile)
        return false;

    TileExtra_Lock* pLockExtra = pLockTile->GetExtra<TileExtra_Lock>();
    if (!pLockExtra || !pLockExtra->HasAccess(GetUserID()))
        return false;

    if (pLockExtra->ownerID == pTarget->GetUserID())
        return false;

    return true;
}

bool GamePlayer::AbleToWorldBanSomeone(GamePlayer* pTarget)
{
    if (!pTarget)
        return false;

    if (!pTarget || m_currentWorldID == 0)
        return false;

    if (m_currentWorldID != pTarget->GetCurrentWorld())
        return false;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
    if (!pWorld)
        return false;

    TileInfo* pWorldLock = pWorld->GetTileManager()->GetKeyTile(KEY_TILE_WORLD_LOCK);
    if (!pWorldLock)
        return false;

    TileExtra_Lock* pLockExtra = pWorldLock->GetExtra<TileExtra_Lock>();
    if (!pLockExtra || !pLockExtra->HasAccess(GetUserID()) || pLockExtra->HasAccess(pTarget->GetUserID()))
        return false;

    return true;
}

void GamePlayer::OnDeath()
{
    if (m_currentWorldID == 0)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
    if (!pWorld)
        return;

    SendOnSetFreezeState(PLAYER_FREEZE_STATE_FROZEN_BUT_GRAVITY, 0);
    SendOnKilled();
    OnDeathSpike();
}

void GamePlayer::OnDeathSpike(int32 tileX, int32 tileY)
{
    if (m_currentWorldID == 0)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
    if (!pWorld)
        return;

    pWorld->SendPositionCorrectionToAll(this, m_respawnPos, 2000);
    pWorld->SendPlayPositionedToAll(this, "teleport.wav");
    SendOnSetFreezeState(PLAYER_FREEZE_STATE_NONE, 2000);
    m_lastDeathTime.Reset();
}

void GamePlayer::OpenPaginatedDialog(std::unique_ptr<DialogPagination> newDialog)
{
    m_dialogPagination = std::move(newDialog);
    if (m_dialogPagination)
    {
        m_dialogPagination->Render(this);
    }
}