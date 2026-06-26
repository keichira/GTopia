#include "WrenchSelfDialog.h"
#include "../GamePlayer.h"
#include "Utils/DialogBuilder.h"
#include "../../World/WorldManager.h"
#include "../../Player/PlayerManager.h"
#include "Math/Math.h"

void WrenchSelfDialog::Request(GamePlayer* pPlayer)
{
    if (!pPlayer)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    PlayerProgress& progressData = pPlayer->GetProgressData();
    PlayerLoginDetail& loginDetail = pPlayer->GetLoginDetail();
    CharacterData& characterData = pPlayer->GetCharData();
    PlayerInventory& inventory = pPlayer->GetInventory();
    PlayerPlayModController& modController = pPlayer->GetModController();

    DialogBuilder db;
    db.SetDefaultColor('o');

    TileInfo* pLockAcessTile = pPlayer->GetLockAcessTile();
    if (pLockAcessTile)
    {
        TileExtra_Lock* pTileExtra = pLockAcessTile->GetExtra<TileExtra_Lock>();
        if (!pTileExtra)
        {
            pPlayer->SetLockAccessTile(-1);
        }
        else
        {
            GamePlayer* pOwner = GetPlayerManager()->GetPlayerByNetID(pPlayer->GetLockAcessOwnerID());
            if (!pOwner || pOwner->GetCurrentWorld() != pPlayer->GetCurrentWorld() || pOwner->GetUserID() != pTileExtra->ownerID)
            {
                pPlayer->SetLockAccessTile(-1);
                pLockAcessTile = nullptr;
            }

            if (pLockAcessTile)
            {
                ItemInfo* pLockItem = GetItemInfoManager()->GetItemByID(pLockAcessTile->GetFG());
                if (pLockItem)
                {
                    db.AddButton("acceptlock", "`2Accept Access on " + pLockItem->name);
                }
            }
        }
    }

    db.AddPlayerInfo(pPlayer->GetDisplayName(true), pPlayer->GetPlayerLevel(), progressData.GetProgress(PLAYER_PROGRESS_XP), pPlayer->GetPlayerNextLevelXP())
    ->AddSpacer();

    uint8 onlineStatus = progressData.GetProgress(PLAYER_PROGRESS_ONLINE_STATUS) & 3;

    if(loginDetail.protocol > 127)
    {
        db.AddCustomButton("title_edit", "image:interface/large/gui_wrench_title.rttex;image_size:400,260;width:0.19;");

        if(onlineStatus == PLAYER_ONLINE_STATUS_DEFAULT)
            db.AddCustomButton("set_online_status", "image:interface/large/gui_wrench_online_status_1green.rttex;image_size:400,260;width:0.19;");
        else if(onlineStatus == PLAYER_ONLINE_STATUS_AWAY)
            db.AddCustomButton("set_online_status", "image:interface/large/gui_wrench_online_status_2yellow.rttex;image_size:400,260;width:0.19;");
        else
            db.AddCustomButton("set_online_status", "image:interface/large/gui_wrench_online_status_3red.rttex;image_size:400,260;width:0.19;");

        if(pPlayer->HasFlag(PLAYER_FLAG_SUPPORTER) || pPlayer->HasFlag(PLAYER_FLAG_SUPER_SUPPORTER))
            db.AddCustomButton("billboard_edit", "image:interface/large/gui_wrench_edit_billboard.rttex;image_size:400,260;width:0.19");

        db.AddCustomButton("notebook_edit", "image:interface/large/gui_wrench_notebook.rttex;image_size:400,260;width:0.19;");
        db.AddCustomButton("goals", "image:interface/large/gui_wrench_goals_quests.rttex;image_size:400,260;width:0.19;");
        db.AddCustomButton("my_worlds", "image:interface/large/gui_wrench_my_worlds.rttex;image_size:400,260;width:0.19;");
        db.AddCustomButton("alist", "image:interface/large/gui_wrench_achievements.rttex;image_size:400,260;width:0.19;");

        if(inventory.GetClothByPart(BODY_PART_HAND) == ITEM_ID_BATTLE_LEASH && progressData.GetProgress(PLAYER_PROGRESS_PET_2_0) != 0)
            db.AddCustomButton("pets", "image:interface/large/gui_wrench_battle_pets.rttex;image_size:400,260;width:0.19;");
    }
    else
    {
        db.AddButton("title_edit", "`$Title``");

        // convert status to textureX
        if(onlineStatus == PLAYER_ONLINE_STATUS_BUSY) onlineStatus = 30;
        else if(onlineStatus = PLAYER_ONLINE_STATUS_AWAY) onlineStatus = 29;
        else onlineStatus = 28;
        db.AddInnerImageLabelButton("set_online_status", "`$Set Online Status``", "game/tiles_page14.rttex", onlineStatus, 23);

        if(pPlayer->HasFlag(PLAYER_FLAG_SUPPORTER) || pPlayer->HasFlag(PLAYER_FLAG_SUPER_SUPPORTER))
            db.AddButton("billboard_edit", "`$Edit Billboard``");

        db.AddButton("notebook_edit", "`$Notebook``");
        db.AddButton("goals", "`$Goals & Quests``");
        // check daily bonus

        db.AddButton("my_worlds", "`$My Worlds``");
        db.AddButton("alist", "`$Challanges (" + ToString(progressData.GetCountOfCompletedAchieves()) + "`5/``" + ToString(ACHIEVEMENT_COUNT) + ")");

        if(inventory.GetClothByPart(BODY_PART_HAND) == ITEM_ID_BATTLE_LEASH && progressData.GetProgress(PLAYER_PROGRESS_PET_2_0) != 0)
            db.AddButton("pets", "`wBattle Pets!``");
    }

    if(modController.GetActiveModCount() > 0)
    {
        db.AddTextBox("`wActive effects:``");
        modController.BuildActiveModsDialog(db);
    }

    db.AddSpacer();
    db.AddTextBox("`oYou have `w" + ToString(inventory.GetInventorySize()) + "`` backpack slots.``");

    Vector2Float vPlayerWorldPos = pPlayer->GetWorldPos();
    int32 posX = vPlayerWorldPos.x / 32;
    int32 posY = vPlayerWorldPos.y / 32;

    string worldInfo = "`oCurrent world: `w" + pWorld->GetWorlName() + "`` (`w" + ToString(posX) + "``, `w" + ToString(posY) + "``) (`w" + ToString(pWorld->GetPlayerCount()) + "`` ";
    if(pWorld->GetPlayerCount() == 1)
        worldInfo += "person)";
    else
        worldInfo += "people)";

    db.AddTextBox(worldInfo);
    db.EndDialog("plyr_wrench", "", "Continue");

    pPlayer->SendOnDialogRequest(db.Get());
}

void WrenchSelfDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if(!pPlayer)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    auto pButtonClicked = packet.Find("buttonClicked"_hash);
    if(!pButtonClicked)
        return;

    if(pButtonClicked->valueSize == 0 || pButtonClicked->valueSize > 35)
        return;

    std::string_view clickedButton = pButtonClicked->GetStringView();

    if(clickedButton == "acceptlock")
    {
        TileInfo* pLockAcessTile = pPlayer->GetLockAcessTile();
        if (!pLockAcessTile)
            return;

        TileExtra_Lock* pTileExtra = pLockAcessTile->GetExtra<TileExtra_Lock>();
        if (!pTileExtra)
            return;

        ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pLockAcessTile->GetFG());
        if (!pItem)
            return;

        DialogBuilder db;
        db.SetDefaultColor('o')
            ->AddLabelWithIcon("Accept Access To World", pItem->id)
            ->AddSmallText("When you have access to a world, you are responsible for anything the world's owners or other admins do.")
            ->AddSmallText("Don't accept access to a world unless it is from people you trust.")
            ->AddSmallText("You can remove your access later by either wrenching the lock, or typing `2/unaccess`` to remove yourself from all locks in the world.")
            ->AddSpacer()
            ->AddSmallText("Are you sure you want to be added to this " + pItem->name)
            ->EndDialog("acceptaccess", "Yes", "No");

        pPlayer->SendOnDialogRequest(db.Get());
        return;
    }

    if(clickedButton == "title_edit")
    {
        RequestTitleEdit(pPlayer);
        return;
    }
}

void WrenchSelfDialog::HandleTitleEdit(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if(!pPlayer)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    PlayerProgress& progressData = pPlayer->GetProgressData();

    if(auto pLegendary = packet.Find("checkbox_legendary_title"_hash))
    {
        bool val;
        if(pLegendary->GetBool(val) != TO_INT_SUCCESS)
            return;
        progressData.ModifyTitleActivation(PLAYER_TITLE_LEGEND, val);
    }

    if(auto pDoctor = packet.Find("checkbox_doctor_title"_hash))
    {
        bool val;
        if(pDoctor->GetBool(val) != TO_INT_SUCCESS)
            return;
        progressData.ModifyTitleActivation(PLAYER_TITLE_DOCTOR, val);
    }

    if(auto pMaxLvl = packet.Find("checkbox_max_level_title"_hash))
    {
        bool val;
        if(pMaxLvl->GetBool(val) != TO_INT_SUCCESS)
            return;
        progressData.ModifyTitleActivation(PLAYER_TITLE_MAX_LVL, val);
    }

    if(auto pMaster = packet.Find("checkbox_master_title"_hash))
    {
        bool val;
        if(pMaster->GetBool(val) != TO_INT_SUCCESS)
            return;
        progressData.ModifyTitleActivation(PLAYER_TITLE_MASTER, val);
    }

    if(auto pG4g = packet.Find("checkbox_g4g_title"_hash))
    {
        bool val;
        if(pG4g->GetBool(val) != TO_INT_SUCCESS)
            return;
        progressData.ModifyTitleActivation(PLAYER_TITLE_G4G, val);
    }

    pWorld->SendNameChangeToAll(pPlayer);
    pWorld->SendOnCountryStateToAll(pPlayer);
}

void WrenchSelfDialog::RequestTitleEdit(GamePlayer* pPlayer)
{
    if(!pPlayer)
        return;

    DialogBuilder db;
    db.SetDefaultColor('o')
    ->AddLabel("Select Title:", true);
    
    PlayerProgress& progressData = pPlayer->GetProgressData();
    if(progressData.GetProgress(PLAYER_PROGRESS_TITLES) == 0)
    {
        db.AddLabel("No Titles Obtained", true);
    }
    else
    {
        if(progressData.HasTitle(PLAYER_TITLE_LEGEND))
            db.AddCheckBox("checkbox_legendary_title", "' of Legend'", progressData.IsTitleActive(PLAYER_TITLE_LEGEND));

        if(progressData.HasTitle(PLAYER_TITLE_DOCTOR))
            db.AddCheckBox("checkbox_doctor_title", "'Dr.'", progressData.IsTitleActive(PLAYER_TITLE_DOCTOR));

        if(progressData.HasTitle(PLAYER_TITLE_MAX_LVL))
            db.AddCheckBox("checkbox_max_level_title", "Level 125", progressData.IsTitleActive(PLAYER_TITLE_MAX_LVL));

        if(progressData.HasTitle(PLAYER_TITLE_MASTER))
            db.AddCheckBox("checkbox_master_title", "Master", progressData.IsTitleActive(PLAYER_TITLE_MASTER));

        if(progressData.HasTitle(PLAYER_TITLE_G4G))
            db.AddCheckBox("checkbox_g4g_title", "Grow4Good Title", progressData.IsTitleActive(PLAYER_TITLE_G4G));
    }

    db.AddSpacer()
    ->AddButton("", "OK")
    ->EndDialog("title_edit", "", "");

    pPlayer->SendOnDialogRequest(db.Get());
}
