#include "WrenchSelfDialog.h"
#include "../GamePlayer.h"
#include "Utils/DialogBuilder.h"
#include "../../World/WorldManager.h"
#include "Math/Math.h"

void WrenchSelfDialog::Request(GamePlayer* pPlayer)
{
    if(!pPlayer)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    PlayerProgress& progressData = pPlayer->GetProgressData();
    PlayerLoginDetail& loginDetail = pPlayer->GetLoginDetail();
    CharacterData& characterData = pPlayer->GetCharData();
    PlayerInventory& inventory = pPlayer->GetInventory();
    PlayerPlayModController& modController = pPlayer->GetModController();

    DialogBuilder db;
    db.SetDefaultColor('o')
    ->AddPlayerInfo(pPlayer->GetDisplayName(true), pPlayer->GetPlayerLevel(), progressData.GetProgress(PLAYER_PROGRESS_XP), pPlayer->GetPlayerNextLevelXP());

    if(loginDetail.protocol > 96)
    {
        if(loginDetail.protocol > 127)
        {
            db.AddCustomButton("title_edit", "image:interface/large/gui_wrench_title.rttex;image_size:400,260;width:0.19;");
        }
        else 
        {
            db.AddButton("title_edit", "`$Title``");
        }
    }
    db.AddButton("goals", "`$Goals & Quests``");
    
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