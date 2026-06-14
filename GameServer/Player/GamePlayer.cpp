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
#include "Dialog/RenderWorldDialog.h"
#include "Dialog/RegisterDialog.h"
#include "Dialog/DropItemDialog.h"
#include "Proton/ProtonUtils.h"
#include "PlayerManager.h"
#include "Math/Math.h"
#include "Utils/GrowUtils.h"

GamePlayer::GamePlayer() 
: m_currentWorldID(0), m_joiningWorld(false), m_guestID(0), 
m_lastItemActivateTime(0), m_state(0), m_flags(0), m_gems(0),
m_progressData(this), m_modController(this), m_activeBattlePetSlot(0)
{
    RandomizeNextDBSaveTime();
}

GamePlayer::~GamePlayer()
{
}

void GamePlayer::SendGems(bool skipAnim)
{
    SendOnSetBux(m_gems, skipAnim, HasFlag(PLAYER_FLAG_SUPPORTER), HasFlag(PLAYER_FLAG_SUPER_SUPPORTER));
}

void GamePlayer::ModifyGems(int32 count, bool sendToPlayer)
{
    if(count == 0)
        return;

    m_gems += count;

    if(count > 0)
    {
        
    }

    if(sendToPlayer)
    {
        SendGems(false);
    }
}

void GamePlayer::GiveXP(uint32 amount)
{
    // XP= 50 * ( LVL2 + 2 )

    uint32 playerLevel = Sqrt((m_progressData.GetProgress(PLAYER_PROGRESS_XP)/50) - 2);

    if(playerLevel == 125)
        return;
    
    m_progressData.AddProgress(PLAYER_PROGRESS_XP, amount);

    uint32 playerNewLevel = Sqrt((m_progressData.GetProgress(PLAYER_PROGRESS_XP)/50) - 2);
    if(playerNewLevel > 125)
    {
        // force level to 125
        playerNewLevel = 125;
        m_progressData.SetProgress(PLAYER_PROGRESS_XP, 50 * (125 * 125 + 2));
    }

    if(playerNewLevel > playerLevel)
    {
        World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
        if(!pWorld)
        {
            SendOnConsoleMessage("You are now level " + ToString(playerNewLevel) + "!");
            return;
        }

        pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_LEVELUP, m_worldPos);
        pWorld->SendTalkBubbleAndConsoleToAll(GetDisplayName(false) + " `wis now level " + ToString(playerNewLevel) + "!", false, this);
    }
}

uint32 GamePlayer::GetPlayerLevel()
{
    uint32 playerXP = m_progressData.GetProgress(PLAYER_PROGRESS_XP);
    if(playerXP < 100)
        return 0;

    return Sqrt((playerXP/50) - 2);
}

uint32 GamePlayer::GetPlayerNextLevelXP()
{
    return 50 * ((GetPlayerLevel() + 1) * (GetPlayerLevel() + 1) + 2);
}

void GamePlayer::StartLoginRequest(ParsedTextPacket<35>& packet)
{
    if(!HasState(PLAYER_STATE_LOGIN_REQUEST))
        return;

    m_logonStartTime.Reset();

    if(!m_loginDetail.Serialize(packet, this, true)) 
    {
        SendLogonFailWithLog("`4HUH?! ``Are you sure everything is alright?");
        LogOff(true, false, true);
        return;
    }

    /*GameConfig* pGameConfig = GetContext()->GetGameConfig();

    float minVersion = 0.0f;
    float maxVersion = 0.0f;

    switch(m_loginDetail.platformType)
    {
        case Proton::PLATFORM_ID_WINDOWS:
        {
            minVersion = pGameConfig->windowsSupportedVersions[0];
            maxVersion = pGameConfig->windowsSupportedVersions[1];
            break;
        }

        case Proton::PLATFORM_ID_ANDROID:
        {
            minVersion = pGameConfig->androidSupportedVersions[0];
            maxVersion = pGameConfig->androidSupportedVersions[1];
            break;
        }

        case Proton::PLATFORM_ID_IOS:
        {
            minVersion = pGameConfig->iosSupportedVersions[0];
            maxVersion = pGameConfig->iosSupportedVersions[1];
            break;
        }

        case Proton::PLATFORM_ID_OSX:
        {
            minVersion = pGameConfig->macosSupportedVersions[0];
            maxVersion = pGameConfig->macosSupportedVersions[1];
            break;
        }
    }

    if(m_loginDetail.gameVersion > maxVersion || m_loginDetail.gameVersion < minVersion)
    {
        SendLogonFailWithLog("`4Oops`o, your version v`5" + ToString(m_loginDetail.gameVersion) + "`o is not supported. Supported versions are v`5" + ToString(minVersion) + "`o-v`5" + ToString(maxVersion) + "`o.");
        LogOff(true, false, true);
        return;
    }*/

    m_userID = m_loginDetail.user;
    GetMasterBroadway()->SendCheckSessionPacket(GetNetID(), m_loginDetail.user, m_loginDetail.token, GetContext()->GetID());
}

void GamePlayer::HandleCheckSession(VariantVector&& result)
{
    bool foundSession = result[2].GetBool();
    if(!foundSession) 
    {
        SendLogonFailWithLog("`4OOPS! ``Please re-connect server says you're not belong to this server");
        return;
    }

    GamePlayer* pTarget = GetPlayerManager()->IsPlayerAlreadyOn(this);
    if(GetPlayerManager()->IsPlayerAlreadyOn(this)) 
    {
        SendOnConsoleMessage("`4ALREADY ON?!`` : This account was already online, kicking it off so you can log on. (if you were just playing before, this is nothing to worry about)");
        pTarget->SendOnConsoleMessage("`4This account is being activated from another device, kicking you off so they can get on");
        pTarget->LogOff(true, true, false);
    }

    if(m_loginDetail.loginMode == LOGON_MODE_TRANSFER) 
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
    if(!itemData)
    {
        SendLogonFailWithLog("`4Oops`o, something went wrong please re-login.");
        LOGGER_LOG_ERROR("Failed to get client data platform: %d, gameversion: %f", m_loginDetail.platformType, m_loginDetail.gameVersion);
        return;
    }

    GameConfig* pGameConfig = GetContext()->GetGameConfig();

    RemoveState(PLAYER_STATE_LOGIN_REQUEST);
    SetState(PLAYER_STATE_ENTERING_GAME);

    SendWelcomePacket(itemData->hash, pGameConfig->cdnServer, pGameConfig->cdnPath, settings, 0);
}

void GamePlayer::HandleRenderWorld(VariantVector&& result)
{
    if(!HasState(PLAYER_STATE_RENDERING_WORLD)) 
        return;

    int32 renderResult = result[2].GetINT();

    if(renderResult == TCP_RESULT_OK) 
    {
        World* pWorld = GetWorldManager()->GetWorldByDatabaseID(result[4].GetUINT());
        if(!pWorld)
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

void GamePlayer::SaveToDatabase()
{
    uint32 invMemSize = m_inventory.GetMemEstimate(true);
    uint8* pInvData = new uint8[invMemSize];
    MemoryBuffer invMemBuffer(pInvData, invMemSize);
    m_inventory.Serialize(invMemBuffer, true, true);

    uint32 progressMemSize = m_progressData.GetMemEstimate();
    uint8* pProgressData = new uint8[progressMemSize];
    MemoryBuffer progressMemBuffer(pProgressData, progressMemSize);
    m_progressData.Serialize(progressMemBuffer, true);

    uint32 worldID = 0;
    if(m_currentWorldID != 0) 
    {
        World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
        if(pWorld) 
        {
            worldID = pWorld->GetDatabaseID();
        }
    }

    uint32 roleID = GetRoleManager()->GetDefaultRoleID();
    if(m_pRole)
    {
        roleID = m_pRole->GetID();
    }
    else
    {
        LOGGER_LOG_WARN("Player %s (%d) SaveDB role is NULL setting default role %d", GetRawName(), GetUserID(), roleID);
    }

    QueryRequest req = PlayerDB::Save(
        m_userID,
        roleID,
        ToHex(pInvData, invMemSize),
        0, //m_characterData.GetSkinColor(),
        m_flags,
        worldID,
        ToHex(pProgressData, progressMemSize),
        m_gems,
        GetNetID()
    );
    
    DatabasePlayerExec(GetContext()->GetDatabasePool(), req);
    SAFE_DELETE_ARRAY(pProgressData);
    SAFE_DELETE_ARRAY(pInvData);
}

void GamePlayer::LogOff(bool forceDelete, bool saveToDb, bool endSession, bool sendNetworkPackets)
{
    if(forceDelete)
    {
        SetState(PLAYER_STATE_DELETE);
    }

    if(!HasState(PLAYER_STATE_LOGGING_OFF)) 
    {
        SetState(PLAYER_STATE_LOGGING_OFF);

        if(HasState(PLAYER_STATE_IN_GAME) && saveToDb) 
        {
            SaveToDatabase();
        }

        if(forceDelete && m_currentWorldID != 0 && HasState(PLAYER_STATE_IN_GAME)) 
        {
            World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
            if(pWorld) {
                pWorld->PlayerLeaveWorld(this, true);
            }
        }

        if(forceDelete && sendNetworkPackets) 
        {
            SendUDPPacket(GetNetID(), NET_MESSAGE_GAME_MESSAGE, "action|logoff\n");
            SendUDPDisconnectPacket(GetNetID());
        }

        SetState(PLAYER_STATE_DELETE);
        
        if(endSession) // todo here
        {
            GetMasterBroadway()->SendEndPlayerSession(m_userID);
        }
    }
}

void GamePlayer::Update()
{
    m_modController.Update();

    if(m_currentWorldID != 0) 
    {
        if(World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID)) 
        {
            if(m_characterData.needCharStateUpdate)
            {
                pWorld->SendSetCharPacketToAll(this);
                m_characterData.needCharStateUpdate = false;
            }

            if(m_characterData.needSkinUpdate) 
            {
                pWorld->SendSkinColorUpdateToAll(this);
                m_characterData.needSkinUpdate = false;
            }
        }
    }

    if(m_logonStartTime.GetElapsedTime() >= 180000 && (HasState(PLAYER_STATE_LOGIN_REQUEST) || HasState(PLAYER_STATE_ENTERING_GAME))) 
    {
        LogOff(true, false, true);
        return;
    }

    if(HasState(PLAYER_STATE_IN_GAME)) 
    {
        /**
         * todo ping request
         */
    }
}

string GamePlayer::GetDisplayName(bool checkWorld)
{
    string displayName;

    if(m_pRole->GetNameColor() != 0) 
    {
        displayName += "`"; 
        displayName += m_pRole->GetNameColor();
    }

    if(checkWorld && m_currentWorldID != 0) 
    {
        World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
        if(pWorld) {
            if(pWorld->IsPlayerWorldOwner(this)) 
            {
                displayName += "`2";
            }
            else if(pWorld->IsPlayerWorldAdmin(this)) 
            {
                displayName += "`^";
            }
        }
    }
    
    displayName += m_pRole->GetPrefix() + GetRawName() + m_pRole->GetSuffix();
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
    spawnData += "name|" + GetDisplayName(true) + "``\n";
    spawnData += "country|" + m_loginDetail.country + "\n";
    spawnData += "invis|0\n"; // todo
    spawnData += "mstate|";
    spawnData += m_pRole->HasPerm("state.mod"_hash) ? "1\n" : "0\n";
    spawnData += "smstate|" ;
    spawnData += m_pRole->HasPerm("state.smod"_hash) ? "1\n" : "0\n";
    spawnData += "onlineID|\n";

    if(local) 
    {
        spawnData += "type|local\n";
    }

    return spawnData;
}

Vector2Float GamePlayer::GetWorldPosCenter()
{
    return Vector2Float(m_worldPos.x + 16, m_worldPos.y + 16);
}

RectFloat GamePlayer::GetPlayerWorldRect()
{
    return RectFloat(m_worldPos.x, m_worldPos.y, m_worldPos.x + 20, m_worldPos.y + 30);
}

void GamePlayer::ToggleCloth(uint16 itemID)
{
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if(!pItem || pItem->bodyPart > BODY_PART_SIZE)
        return;

    if((pItem->type != ITEM_TYPE_CLOTHES && pItem->type != ITEM_TYPE_ARTIFACT))
        return;

    if(pItem->type == ITEM_TYPE_ARTIFACT)
        /**
         * 
         */
        return;

    uint16 wornItem = m_inventory.GetClothByPart((eBodyPart)pItem->bodyPart);
    if(wornItem == pItem->id) 
    {
        m_inventory.SetClothByPart(ITEM_ID_BLANK, pItem->bodyPart);

        if(pItem->playModType != PLAYMOD_TYPE_NONE) 
        {
            m_modController.RemovePlayMod(pItem->playModType);
        }

        PlayerInventory& playerInv = GetInventory();

        uint8 itemCount = playerInv.GetCountOfItem(pItem->id);
        switch(pItem->id)
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
    else {
        m_inventory.SetClothByPart(pItem->id, pItem->bodyPart);

        ItemInfo* pWornItem = GetItemInfoManager()->GetItemByID(wornItem);
        if(pWornItem)
        {
            m_modController.RemovePlayMod(pWornItem->playModType);
        }

        if(pItem->playModType != PLAYMOD_TYPE_NONE) 
        {
            m_modController.AddPlayMod(pItem->playModType);
        }
    }

    if(m_currentWorldID != 0) 
    {
        World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
        pWorld->SendClothUpdateToAll(this);
    }
}

void GamePlayer::ToggleBattlePetLeash(bool forceFirstSlot)
{
    if(!forceFirstSlot)
    {
        m_activeBattlePetSlot = 1 - m_activeBattlePetSlot;
    }

    if(m_activeBattlePetSlot == 1)
    {
        if(m_progressData.GetProgress(PLAYER_PROGRESS_PET_2_0) == 0)
        {
            m_activeBattlePetSlot = 0;
        }
    }

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
    if(!pWorld)
        return;

    int32 petID = 0;
    if(m_activeBattlePetSlot == 0)
    {
        petID = m_progressData.GetProgress(PLAYER_PROGRESS_PET_1_0);
    }
    else
    {
        petID = m_progressData.GetProgress(PLAYER_PROGRESS_PET_2_0);
    }

    pWorld->SendBattlePetPacketToAll(PET_EVENT_EQUIP, GetNetID(), petID);
}

void GamePlayer::ModifyInventoryItem(uint16 itemID, int16 amount)
{
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if(!pItem || amount == 0)
        return;

    if(amount > 0 && IsIllegalItem(itemID) && !m_pRole->HasPerm("bypass.item_illegal"_hash))
        return;

    if(amount > 0 && pItem->HasFlag(ITEM_FLAG_MOD) && !m_pRole->HasPerm("bypass.item_mod"_hash))
        return;

    if(amount < 0) 
    {
        m_inventory.RemoveItem(itemID, -amount, this);

        if(m_inventory.GetCountOfItem(itemID) == 0 && m_inventory.IsWearingItem(itemID))
        {
            ToggleCloth(itemID);
        }
    }
    else
    {
        m_inventory.AddItem(itemID, amount, this);
    }
}

void GamePlayer::TrashItem(uint16 itemID, uint16 amount)
{
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if(!pItem) 
    {
        LOGGER_LOG_WARN("Player %d tried to trash non exist item %d ?!", m_userID, itemID)
        return;
    }

    if(amount > pItem->maxCanHold)
        return;

    if(amount > m_inventory.GetCountOfItem(itemID)) 
    {
        PlaySFX("cant_place_tile.wav");
        return;
    }

    ModifyInventoryItem(itemID, -amount);

    PlaySFX("trash.vaw");
    SendOnConsoleMessage("Trashed " + ToString(amount) + " " + pItem->name);
}

void GamePlayer::DropItem(uint16 itemID, uint16 amount, bool openDialog)
{
    if(m_currentWorldID == 0)
        return;

    InventoryItemInfo* pInvItem = m_inventory.GetItemByID(itemID);
    if(!pInvItem)
        return;

    if(amount > pInvItem->count)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if(!pItem)
        return;

    if(pItem->type == ITEM_TYPE_PETFISH && pInvItem->count != amount)
    {
        SendOnTalkBubble("Please don't chop up the fish", true);
        return;
    }

    if(pItem->maxCanHold == 0)
    {
        SendOnTalkBubble("You can't drop that.", true);
        PlaySFX("cant_place_tile.wav");
        return;
    }

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
    if(!pWorld)
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
    if(!pTile || pTile->IsCollidable())
    {
        SendOnTalkBubble("You can't drop that here, face somewhere with open space.", true);
        PlaySFX("cant_place_tile.wav");
        return;
    }

    Vector2Float vTileWorldPos = pTile->GetWorldPos();
    dropPos.x = Clamp(dropPos.x, vTileWorldPos.x + 2.f, vTileWorldPos.x + 29.f);
    dropPos.y = Clamp(dropPos.y, vTileWorldPos.y + 2.f, vTileWorldPos.y + 29.f);

    if(pWorld->GetObjectManager()->GetCounfOfObjestsInRect(pTile->GetRect()) > 19)
    {
        SendOnTalkBubble("You can't drop that here, find an emptier spot!", true);
        PlaySFX("cant_place_tile.wav");
        return;
    }

    if(IsMainDoor(pTile->GetDisplayedItem()))
    {
        SendOnTalkBubble("You can't drop items on the white door.", true);
        PlaySFX("audio/cant_place_tile.wav");
        return;
    }

    if(openDialog)
    {
        DropItemDialog::Request(this, pInvItem);
        return;
    }

    ModifyInventoryItem(itemID, -amount);
    pWorld->DropObjectOnTile(pTile, itemID, amount, dropPos - pTile->GetWorldPosCenter(), true);
}

void GamePlayer::CheckLimitsForAccountCreation(bool fromDialog, const VariantVector& extraData)
{
    QueryRequest req;

    if(m_loginDetail.platformType == Proton::PLATFORM_ID_WINDOWS && !m_loginDetail.sid.empty()) 
    {
        req = PlayerDB::CountBySidMacIP(m_loginDetail.sid, m_loginDetail.mac, GetAddress(), GetNetID());
    }
    else if(m_loginDetail.platformType == Proton::PLATFORM_ID_ANDROID && !m_loginDetail.gid.empty()) 
    {
        req = PlayerDB::CountByGidMacIP(m_loginDetail.gid, m_loginDetail.mac, GetAddress(), GetNetID());
    }
    else if(m_loginDetail.platformType == Proton::PLATFORM_ID_IOS && !m_loginDetail.vid.empty()) 
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

void GamePlayer::CheckAccountCreationLimitCB(QueryTaskResult&& result)
{
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(result.ownerID);
    if(!pPlayer || !result.result)
        return;

    Variant* pMac = result.result->GetFieldSafe("mac_count", 0);
    Variant* pIP = result.result->GetFieldSafe("ip_count", 0);
    Variant* pOther = nullptr;
    bool shouldSetOther = true;

    PlayerLoginDetail& loginDetail = pPlayer->GetLoginDetail();

    if(loginDetail.platformType == Proton::PLATFORM_ID_WINDOWS && !loginDetail.sid.empty()) 
    {
        pOther = result.result->GetFieldSafe("sid_count", 0);
    }
    else if(loginDetail.platformType == Proton::PLATFORM_ID_ANDROID && !loginDetail.gid.empty()) 
    {
        pOther = result.result->GetFieldSafe("gid_count", 0);
    }
    else if(loginDetail.platformType == Proton::PLATFORM_ID_IOS && !loginDetail.vid.empty()) 
    {
        pOther = result.result->GetFieldSafe("vid_count", 0);
    }
    else 
    {
        shouldSetOther = false;
    }

    if(!pMac && !pIP && (!shouldSetOther || !pOther)) 
    {
        pPlayer->SendOnTalkBubble("`4Oops! ``Something went wrong while checking for account creating.", true);
        return;
    }

    GameConfig* pGameConfig = GetContext()->GetGameConfig();
    if(
        pIP->GetUINT() > pGameConfig->maxAccountsPerIP ||
        pMac->GetUINT() > pGameConfig->maxAccountsPerMac ||
        (shouldSetOther && (
            pOther->GetUINT() >
            (
                loginDetail.platformType == Proton::PLATFORM_ID_WINDOWS ? pGameConfig->maxAccountsPerSid :
                loginDetail.platformType == Proton::PLATFORM_ID_ANDROID ? pGameConfig->maxAccountsPerGid :
                pGameConfig->maxAccountsPerVid
            )
        ))
    ) {
        pPlayer->SendOnTalkBubble("`4Oops! ``You've reached the max `5GrowID ``accounts you can make for this device or IP address!", true);
        return;
    }

    bool fromDialog = result.extraData[0].GetBool();
    if(fromDialog) 
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
    if(!pPlayer || !result.result)
        return;
    
    if(result.result->GetRowCount() > 0) 
    {
        RegisterDialog::Request(
            pPlayer, result.extraData[1].GetString(),
            result.extraData[2].GetString(), result.extraData[3].GetString(),
            "`4Oops!`` The name `w" + result.extraData[1].GetString() + "`` is so cool someone else has already taken it. Please choose a different name."
        );
    }
    else 
    {
        QueryRequest req = PlayerDB::GrowIDCreate(pPlayer->GetUserID(), result.extraData[1].GetString(), result.extraData[2].GetString(), pPlayer->GetNetID());
        req.AddExtraData(result.extraData[1].GetString(), result.extraData[2].GetString());

        req.callback = &GamePlayer::CreateAccountFinalCB;
        DatabasePlayerExec(GetContext()->GetDatabasePool(), req);
    }
}

void GamePlayer::CreateAccountFinalCB(QueryTaskResult&& result)
{
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(result.ownerID);
    if(!pPlayer)
        return;

    pPlayer->GetLoginDetail().tankIDName = result.extraData[0].GetString();
    pPlayer->GetLoginDetail().tankIDPass = result.extraData[1].GetString();

    RegisterDialog::Success(pPlayer, result.extraData[0].GetString(), result.extraData[1].GetString());
}

void GamePlayer::SendPositionToWorldPlayers()
{
    if(m_currentWorldID == 0)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
    if(!pWorld)
        return;

    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_STATE;
    packet.field_8.x = m_worldPos.x;
    packet.field_8.y = m_worldPos.y;
    packet.field_4 = GetNetID();

    if(m_characterData.HasCharFlag(CHARACTER_FLAG_FACING_LEFT)) 
    {
        packet.SetFlag(GAME_PACKET_FLAG_FACING_LEFT);
    }

    pWorld->SendGamePacketToAll(&packet, this);
}

float GamePlayer::GetDistToTile(TileInfo* pGoalTile)
{
    if(!pGoalTile)
        return 0.0f;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
    if(!pWorld)
        return 0.0f;

    TileInfo* pPlayerTile = pWorld->GetTileManager()->GetTileByWorldPos(m_worldPos);
    if(!pPlayerTile)
        return 0.0f;

    Vector2Float vPlayerTilePos = pPlayerTile->GetWorldPos();
    Vector2Float vGoalPos = pGoalTile->GetWorldPos();

    return Sqrt((vPlayerTilePos.x - vGoalPos.x)*(vPlayerTilePos.x - vGoalPos.x) + (vPlayerTilePos.y - vGoalPos.y)*(vPlayerTilePos.y - vGoalPos.y));
}

uint32 GamePlayer::GetDistToTileInTiles(TileInfo* pGoalTile)
{
    return GetDistToTile(pGoalTile) / 32;
}
