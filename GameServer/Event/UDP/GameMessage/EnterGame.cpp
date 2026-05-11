#include "EnterGame.h"
#include "Database/Table/PlayerDBTable.h"
#include "../../../Context.h"
#include "../../../Player/PlayerManager.h"
#include "Player/RoleManager.h"
#include "IO/Log.h"
#include "Item/ItemInfoManager.h"
#include "../../../World/WorldManager.h"
#include "Database/Table/WorldDBTable.h"

void CheckAndSendToWorldIfPossible(QueryTaskResult&& result)
{
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(result.ownerID);
    if(!pPlayer)
        return;

    pPlayer->GetLoginDetail().loginMode = LOGON_MODE_TRANSFER;;
    if(!result.result) 
    {
        //GetWorldManager()->SendWorldSelectMenu(pPlayer);
        pPlayer->SendOnRequestWorldSelectMenu("");
        return;
    }

    string worldName = result.result->GetField("Name", 0).GetString();

    pPlayer->RemoveState(PLAYER_STATE_ENTERING_GAME);
    pPlayer->SetState(PLAYER_STATE_IN_GAME);
    
    GetWorldManager()->PlayerJoinRequest(pPlayer, worldName);
}

void LoadAccount(QueryTaskResult&& result)
{
    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(result.ownerID);
    if(!pPlayer)
        return;

    if(!result.result) 
    {
        pPlayer->SendLogonFailWithLog("`4OOPS! ``Something went wrong please re-connect");
        return;
    }

    PlayerLoginDetail& loginDetail = pPlayer->GetLoginDetail();
    if(!loginDetail.tankIDName.empty()) 
    {
        loginDetail.tankIDName = result.result->GetField("Name", 0).GetString();
    }

    uint32 roleID = result.result->GetField("RoleID", 0).GetUINT();
    if(roleID == 0) 
    {
        roleID = GetRoleManager()->GetDefaultRoleID();
    }

    Role* pRole = GetRoleManager()->GetRole(roleID);
    if(!pRole) 
    {
        pPlayer->SendLogonFailWithLog("`4OOPS! ``Something went wrong while setting you up, please re-connect");
        LOGGER_LOG_WARN("Failed to set player role %d for user %d", roleID, pPlayer->GetUserID());
        return;
    }
    pPlayer->SetRole(pRole);

    uint32 skinColor = result.result->GetField("SkinColor", 0).GetUINT();
    if(skinColor != 0) 
    {
        pPlayer->GetCharData().SetSkinColor(skinColor);
    }

    pPlayer->SetFlags(result.result->GetField("Flags", 0).GetUINT());
    pPlayer->SetGems(result.result->GetField("Gems", 0).GetUINT());

    PlayerInventory& inventory = pPlayer->GetInventory();

    inventory.SetVersion(pPlayer->GetLoginDetail().protocol);
    string dbInv = result.result->GetField("Inventory", 0).GetString();

    if(!dbInv.empty()) 
    {
        uint32 invMemEstimate = dbInv.size() / 2;
        uint8* pInvData = new uint8[invMemEstimate];

        HexToBytes(dbInv, pInvData);

        MemoryBuffer invMemBuffer(pInvData, invMemEstimate);
        inventory.Serialize(invMemBuffer, false, true);

        SAFE_DELETE_ARRAY(pInvData);
    }

    string progressData = result.result->GetField("ProgressData", 0).GetString();
    if(!progressData.empty())
    {
        uint32 progressMemEstimate = progressData.size() / 2;
        uint8* pProgressData = new uint8[progressMemEstimate];

        HexToBytes(progressData, pProgressData);

        MemoryBuffer progressMemBuffer(pProgressData, progressMemEstimate);
        pPlayer->GetProgressData().Serialize(progressMemBuffer, false);

        SAFE_DELETE_ARRAY(pProgressData);
    }

    if(inventory.GetCountOfItem(ITEM_ID_FIST) == 0) 
    {
        inventory.AddItem(ITEM_ID_FIST, 1);
    }

    if(inventory.GetCountOfItem(ITEM_ID_WRENCH) == 0) 
    {
        inventory.AddItem(ITEM_ID_WRENCH, 1);
    }

    pPlayer->SetGuestID(result.result->GetField("GuestID", 0).GetUINT());

    for(uint8 i = 0; i < BODY_PART_SIZE; ++i) 
    {
        uint16 cloth = inventory.GetClothByPart((eBodyPart)i);

        ItemInfo* pItem = GetItemInfoManager()->GetItemByID(cloth);
        if(!pItem) {
            continue;
        }

        if(pItem->type == ITEM_TYPE_CLOTHES && pItem->playModType != PLAYMOD_TYPE_NONE) {
            pPlayer->AddPlayMod(pItem->playModType, true);
        }
    }

    if(loginDetail.loginMode == LOGON_MODE_WELCOME) 
    {
        pPlayer->SendOnConsoleMessage("Welcome back, `w" + pPlayer->GetDisplayName() + "`o.");
    }

    pPlayer->SendGems(true);
    pPlayer->SendSetHasGrowID(pPlayer->HasGrowID() ? true : false);
    pPlayer->SendInventoryPacket();

    uint32 worldID = result.result->GetField("LastWorld", 0).GetUINT();

    if(worldID != 0 && loginDetail.loginMode == LOGON_MODE_WELCOME) 
    {
        QueryRequest req = WorldDB::GetByID(worldID, pPlayer->GetNetID());
        req.callback = &CheckAndSendToWorldIfPossible;
        DatabaseWorldExec(GetContext()->GetDatabasePool(), req);
        return;
    }

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(pWorld) 
    {
        pPlayer->RemoveState(PLAYER_STATE_ENTERING_GAME);
        pPlayer->SetState(PLAYER_STATE_IN_GAME);

        GetWorldManager()->PlayerJoinRequest(pPlayer, pWorld->GetWorlName());
    }
    else 
    {
        pPlayer->RemoveState(PLAYER_STATE_ENTERING_GAME);
        pPlayer->SetState(PLAYER_STATE_IN_GAME);
        pPlayer->SendOnRequestWorldSelectMenu("");
    }
}

void EnterGame::Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    if(!pPlayer || !pPlayer->HasState(PLAYER_STATE_ENTERING_GAME))
        return;

    QueryRequest req = PlayerDB::GetData(pPlayer->GetUserID(), pPlayer->GetNetID());
    req.callback = &LoadAccount;
    DatabasePlayerExec(GetContext()->GetDatabasePool(), req);
}