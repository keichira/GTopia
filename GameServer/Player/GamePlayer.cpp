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

GamePlayer::GamePlayer(ENetPeer* pPeer) 
: Player(pPeer), m_currentWorldID(0), m_joiningWorld(false), m_guestID(0), 
m_lastItemActivateTime(0), m_state(0), m_flags(0), m_gems(0),
m_progressData(this)
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
        pWorld->SendTalkBubbleAndConsoleToAll(GetDisplayName() + " ``is now level " + ToString(playerNewLevel) + "!", true, this);
    }
}

void GamePlayer::StartLoginRequest(ParsedTextPacket<25>& packet)
{
    if(!HasState(PLAYER_STATE_LOGIN_REQUEST))
        return;

    m_logonStartTime.Reset();

    if(!m_loginDetail.Serialize(packet, this, true)) 
    {
        SendLogonFailWithLog("`4HUH?! ``Are you sure everything is alright?");
        return;
    }

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
    settings += "proto=144"; /** search it what it effects in client */
    settings += "|server_tick=" + ToString(Time::GetSystemTime());
    settings += "|choosemusic=audio/mp3/about_theme.mp3";
    settings += "|usingStoreNavigation=1";
    settings += "|enableInventoryTab=1";

    auto itemData = GetItemInfoManager()->GetClientData(m_loginDetail.platformType, m_loginDetail.gameVersion);
    auto tributeData = GetPlayerTributeManager()->GetClientData(m_loginDetail.protocol);
    GameConfig* pGameConfig = GetContext()->GetGameConfig();

    RemoveState(PLAYER_STATE_LOGIN_REQUEST);
    SetState(PLAYER_STATE_ENTERING_GAME);
    SendWelcomePacket(itemData->hash, pGameConfig->cdnServer, pGameConfig->cdnPath, settings, tributeData->hash);
}

void GamePlayer::HandleRenderWorld(VariantVector&& result)
{
    if(!HasState(PLAYER_STATE_RENDERING_WORLD)) 
    {
        return;
    }

    int32 renderResult = result[2].GetINT();

    if(renderResult == TCP_RESULT_OK) 
    {
        string worldName = result[4].GetString();
        SendOnConsoleMessage("`oYour world \"`#" + worldName + "`o\" has been rendered!");

        RenderWorldDialog::OnRendered(this, worldName);
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

    QueryRequest req = PlayerDB::Save(
        m_userID,
        m_pRole->GetID(),
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

void GamePlayer::LogOff(bool forceDelete, bool saveToDb, bool endSession)
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
                pWorld->PlayerLeaverWorld(this);
            }
        }

        if(forceDelete && m_pPeer->state != ENET_PEER_STATE_DISCONNECTED) 
        {
            SendENetPacket(NET_MESSAGE_GAME_MESSAGE, "action|logoff\n", m_pPeer);
            enet_peer_disconnect(m_pPeer, 0);
        }

        SetState(PLAYER_STATE_DELETE);
        
        if(endSession) 
        {
            GetMasterBroadway()->SendEndPlayerSession(m_userID);
        }
    }
}

void GamePlayer::Update()
{
    UpdatePlayMods();

    if(m_currentWorldID != 0) 
    {
        World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
        if(pWorld) 
        {
            if(m_characterData.NeededCharStateUpdate()) 
            {
                pWorld->SendSetCharPacketToAll(this);
                m_characterData.SetNeedCharStateUpdate(false);
            }

            if(m_characterData.NeededSkinUpdate()) 
            {
                pWorld->SendSkinColorUpdateToAll(this);
                m_characterData.SetNeedSkinUpdate(false);
            }
        }
    }

    if(m_logonStartTime.GetElapsedTime() >= 60000 && (HasState(PLAYER_STATE_LOGIN_REQUEST) || HasState(PLAYER_STATE_ENTERING_GAME))) 
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

string GamePlayer::GetDisplayName()
{
    string displayName;

    if(m_pRole->GetNameColor() != 0) 
    {
        displayName += "`"; 
        displayName += m_pRole->GetNameColor();
    }

    if(m_currentWorldID != 0) 
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
    spawnData += "name|" + GetDisplayName() + "``\n";
    spawnData += "country|" + m_loginDetail.country + "\n";
    spawnData += "invis|0\n"; // todo
    spawnData += "mstate|";
    spawnData += m_pRole->HasPerm(ROLE_PERM_MSTATE) ? "1\n" : "0\n";
    spawnData += "smstate|" ;
    spawnData += m_pRole->HasPerm(ROLE_PERM_SMSTATE) ? "1\n" : "0\n";
    spawnData += "onlineID|\n";

    if(local) 
    {
        spawnData += "type|local\n";
    }

    return spawnData;
}

void GamePlayer::ToggleCloth(uint16 itemID)
{
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if(!pItem || pItem->bodyPart > BODY_PART_SIZE)
        return;

    if((pItem->type != ITEM_TYPE_CLOTHES && pItem->type != ITEM_TYPE_ARTIFACT) && !m_pRole->HasPerm(ROLE_PERM_CAN_WEAR_ANY))
        return;

    if(pItem->type == ITEM_TYPE_ARTIFACT)
        /**
         * 
         */
        return;

    uint16 wornItem = m_inventory.GetClothes()[pItem->bodyPart];
    if(wornItem == pItem->id) 
    {
        m_inventory.SetClothByPart(ITEM_ID_BLANK, pItem->bodyPart);

        if(pItem->playModType != PLAYMOD_TYPE_NONE) 
        {
            RemovePlayMod(pItem->playModType);
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
        }
    }
    else {
        m_inventory.SetClothByPart(pItem->id, pItem->bodyPart);

        ItemInfo* pWornItem = GetItemInfoManager()->GetItemByID(wornItem);
        if(pWornItem) 
        {
            RemovePlayMod(pWornItem->playModType);
        }

        if(pItem->playModType != PLAYMOD_TYPE_NONE) 
        {
            AddPlayMod(pItem->playModType);
        }
    }

    if(m_currentWorldID != 0) 
    {
        World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
        pWorld->SendClothUpdateToAll(this);
    }
}

void GamePlayer::ModifyInventoryItem(uint16 itemID, int16 amount)
{
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if(!pItem || amount == 0)
        return;

    if(amount > 0 && IsIllegalItem(itemID) && !m_pRole->HasPerm(ROLE_PERM_BYPASS_ILLEGAl_ITEM))
        return;

    if(amount > 0 && pItem->HasFlag(ITEM_FLAG_MOD) && !m_pRole->HasPerm(ROLE_PERM_USE_ITEM_TYPE_MOD))
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

    Vector2Int tilePos = pTile->GetPos();
    //dropPos.x = Clamp(dropPos.x, tilePos.x * 32.f + 2.f, tilePos.x * 32.f + 29.f);
    //dropPos.y = Clamp(dropPos.y, tilePos.y * 32.f + 2.f, tilePos.y * 32.f + 29.f);

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
    pWorld->DropObjectOnTile(pTile, itemID, amount, random, true);
}

void GamePlayer::AddPlayMod(ePlayModType modType, bool silent)
{
    if(modType == PLAYMOD_TYPE_NONE)
        return;

    PlayMod* pPlayMod = m_characterData.AddPlayMod(modType);
    if(!pPlayMod)
        return;

    if(!silent) {
        if(pPlayMod->GetTime() != 0) 
        {
            SendOnConsoleMessage(
                "`o" + pPlayMod->GetName() + " (`$" + pPlayMod->GetAddMessage() + " `omod added, `$" + Time::ConvertTimeToStr(pPlayMod->GetTime() * 1000) + "`oleft)"
            );
        }
        else 
        {
            SendOnConsoleMessage("`o" + pPlayMod->GetName() + " (`$" + pPlayMod->GetAddMessage() + " `omod added)");
        }
    }

    UpdateNeededPlayModThings();
}

void GamePlayer::RemovePlayMod(ePlayModType modType, bool silent)
{
    if(modType == PLAYMOD_TYPE_NONE)
        return;

    PlayMod* pPlayMod = m_characterData.RemovePlayMod(modType);
    if(!pPlayMod)
        return;

    if(!silent) 
    {
        SendOnConsoleMessage("`o" + pPlayMod->GetName() + " (`$" + pPlayMod->GetRemoveMessage() + " `omod removed)");
    }

    UpdateNeededPlayModThings();
}

void GamePlayer::UpdateNeededPlayModThings()
{
    if(
        m_characterData.NeededSkinUpdate() && m_currentWorldID != 0
    ) {
        World* pWorld = GetWorldManager()->GetWorldByInstanceID(m_currentWorldID);
        if(!pWorld)
            return;

        if(m_characterData.NeededSkinUpdate()) 
        {
            pWorld->SendSkinColorUpdateToAll(this);
            m_characterData.SetNeedSkinUpdate(false);
        }
    }
}

void GamePlayer::UpdatePlayMods()
{
    auto& reqUpdtMods = m_characterData.GetReqUpdatePlayMods();

    for(int32 i = reqUpdtMods.size() - 1; i >= 0; --i) 
    {
        PlayerPlayModInfo& playMod = reqUpdtMods[i];
        
        if(playMod.modType == PLAYMOD_TYPE_CARRYING_A_TORCH) 
        {
            if(m_currentWorldID == 0)
                continue;

            if(GetInventory().GetClothByPart(BODY_PART_HAND) != ITEM_ID_HAND_TORCH) 
            {
                RemovePlayMod(PLAYMOD_TYPE_CARRYING_A_TORCH);
                continue;
            }

            UpdateTorchPlayMod();
        }
        else if(reqUpdtMods[i].timer.GetElapsedTime() >= playMod.durationMS) 
        {
            RemovePlayMod(playMod.modType);
        }
    }
}

void GamePlayer::UpdateTorchPlayMod()
{
    if(RandomRangeInt(0, 250) != 18) { // is it a good idea? lol since tick based
        return;
    }

    ModifyInventoryItem(ITEM_ID_HAND_TORCH, -1);

    uint8 leftTorchCount = GetInventory().GetCountOfItem(ITEM_ID_HAND_TORCH);
    if(leftTorchCount == 0) 
    {
        SendOnTalkBubble("`2My torch went out!", true);
        return;
    }

    SendOnTalkBubble("`2My torch went out, i have " + ToString(leftTorchCount) + " more!", true);
}

void GamePlayer::SetSkinColor(uint32 skinColor)
{
    m_characterData.SetSkinColor(skinColor);

    if(m_currentWorldID != 0) 
    {
        m_characterData.SetNeedSkinUpdate(true);
    }
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
    packet.posX = m_worldPos.x;
    packet.posY = m_worldPos.y;
    packet.netID = GetNetID();

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
