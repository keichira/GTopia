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

    db.AddPlayerInfo(pPlayer->GetDisplayName(true), pPlayer->GetPlayerLevel(), progressData.GetProgress(PLAYER_PROGRESS_XP), pPlayer->GetPlayerNextLevelXP());

    if (loginDetail.protocol > 96)
    {
        if (loginDetail.protocol > 127)
        {
            db.AddCustomButton("title_edit", "image:interface/large/gui_wrench_title.rttex;image_size:400,260;width:0.19;");
        }
        else
        {
            db.AddButton("title_edit", "`$Title``");
        }
    }
    db.AddButton("goals", "`$Goals & Quests``");

    if (modController.GetActiveModCount() > 0)
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
    if (pWorld->GetPlayerCount() == 1)
        worldInfo += "person)";
    else
        worldInfo += "people)";

    db.AddTextBox(worldInfo);
    db.EndDialog("plyr_wrench", "", "Continue");

    pPlayer->SendOnDialogRequest(db.Get());
}

void WrenchSelfDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pButtonClicked = packet.Find("buttonClicked"_hash);
    if (!pButtonClicked)
        return;

    if (pButtonClicked->valueSize == 0 || pButtonClicked->valueSize > 35)
        return;

    std::string_view clickedButton = pButtonClicked->GetStringView();

    if (clickedButton == "acceptlock")
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
}
