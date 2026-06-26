#include "DoorDialog.h"
#include "Item/ItemInfoManager.h"
#include "../../World/WorldManager.h"
#include "../GamePlayer.h"
#include "Utils/DialogBuilder.h"

void DoorDialog::Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem)
{
    if(!pPlayer || !pTile || !pItem)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    if(!pTile->IsTileExtraType(TILE_EXTRA_TYPE_DOOR))
        return;

    if(!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        return;

    TileExtra_Door* pTileExtra = pTile->GetExtra<TileExtra_Door>();
    if(!pTileExtra)
        return;

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
    ->AddLabelWithIcon("`wEdit " + pItem->name, pItem->id, true)
    ->EmbedData("tilex", vTilePos.x)
    ->EmbedData("tiley", vTilePos.y)
    ->AddTextInput("door_name", "Label", pTileExtra->name, 100)
    ->AddTextInput("door_target", "Destination", pTileExtra->text, 24)
    ->AddSmallText("Enter a Destination in this format: `2WORLDNAME:ID``")
    ->AddSmallText("Leave `2WORLDNAME`` blank (:ID) to go to the door with `2ID`` in the `2Current World``.");

    if(pTile->GetFG() == ITEM_ID_PASSWORD_DOOR || pTile->GetFG() == ITEM_ID_HAUNTED_DOOR)
        db.AddTextInput("door_id", "Password", pTileExtra->id, 23);
    else
    {
        db.AddTextInput("door_id", "ID", pTileExtra->id, 11)
        ->AddSmallText("Set a unique `2ID`` to target this door as a Destination from another!");
    }

    if(pWorld->GetTileManager()->IsTileLockedWithLock(pTile))
    {
        db.AddCheckBox("checkbox_locked", "Is open to public", !pTileExtra->HasFlag(TILE_EXTRA_LOCKED));
    }

    db.EndDialog("door_edit", "OK", "Cancel");
    pPlayer->SendOnDialogRequest(db.Get());
}

void DoorDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if(!pPlayer)
        return;

    auto pTileX = packet.Find("tilex"_hash);
    if(!pTileX)
        return;

    auto pTileY = packet.Find("tiley"_hash);
    if(!pTileY)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    int32 tileX = 0;
    if(pTileX->GetInt(tileX) != TO_INT_SUCCESS)
        return;

    int32 tileY = 0;
    if(pTileY->GetInt(tileY) != TO_INT_SUCCESS)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if(!pTile)
        return;

    if(IsMainDoor(pTile->GetFG()))
        return;

    TileExtra_Door* pTileExtra = pTile->GetExtra<TileExtra_Door>();
    if(!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The door is gone!", false);
        return;
    }

    auto pDoorName = packet.Find("door_name"_hash);
    auto pDoorTarget = packet.Find("door_target"_hash);
    auto pDoorID = packet.Find("door_id"_hash);

    if(pDoorTarget && pDoorTarget->valueSize > 24)
    {
        pPlayer->SendOnTalkBubble("That target world name is too long!", false);
        return;
    }

    if(pDoorName && pDoorName->valueSize > 100)
        return;

    if(pDoorName && pDoorName->GetStringView().find("__&%@PL@%&__") != string::npos)
    {
        pPlayer->SendOnTalkBubble("You try to write the magic symbols down but they disappear!", false);
        return;
    }

    uint32 idLimit = (pTile->GetFG() == ITEM_ID_PASSWORD_DOOR || pTile->GetFG() == ITEM_ID_HAUNTED_DOOR ? 24 : 12);
    if(pDoorID && pDoorID->valueSize > idLimit)
    {
        if(pTile->GetFG() == ITEM_ID_PASSWORD_DOOR || pTile->GetFG() == ITEM_ID_HAUNTED_DOOR)
            pPlayer->SendOnTalkBubble("That password is too long!", false);
        else
            pPlayer->SendOnTalkBubble("That door ID is too long!", false);
        return;
    }

    string doorTarget;
    if(pDoorTarget)
    {
        doorTarget = ToUpper(pDoorTarget->GetString());
        RemoveGTColorCodes(doorTarget);
    }

    if(!doorTarget.empty() && !IsValidWorldName(doorTarget, true))
    {
        pPlayer->SendOnTalkBubble("Sorry, spaces and special characters are not allowed in world or door names.", false);
        return;
    }

    string doorID;
    if(pDoorID)
    {
        doorID = ToUpper(pDoorID->GetString());
        RemoveGTColorCodes(doorID);
    }

    if(!doorID.empty() && !IsValidWorldName(doorID))
    {
        pPlayer->SendOnTalkBubble("Sorry, spaces and special characters are not allowed in world or door names.", false);
        return;
    }

    string doorName;
    if(pDoorName)
    {
        doorName = pDoorName->GetString();
    }

    if(pTile->GetFG() == ITEM_ID_PASSWORD_DOOR || pTile->GetFG() == ITEM_ID_HAUNTED_DOOR && doorName.empty())
    {
        doorName = "Password Door";
    }

    if(pWorld->GetTileManager()->IsTileLockedWithLock(pTile))
    {
        if(auto pIsLocked = packet.Find("checkbox_locked"_hash))
        {
            bool val;
            if(pIsLocked->GetBool(val) != TO_INT_SUCCESS)
                return;

            val ? pTileExtra->RemoveFlag(TILE_EXTRA_LOCKED)
                : pTileExtra->SetFlag(TILE_EXTRA_LOCKED);
        }
    }

    pTileExtra->name = doorName;
    pTileExtra->text = doorTarget;
    pTileExtra->id = doorID;

    pWorld->SendTileUpdate(pTile);
}

void DoorDialog::RequestPasswordDoor(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem)
{
    if(!pPlayer || !pTile || !pItem)
        return;

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
    ->AddLabelWithIcon(pItem->name, pItem->id, true)
    ->EmbedData("tilex", vTilePos.x)
    ->EmbedData("tiley", vTilePos.y)
    ->AddTextBox("The door requires a password.")
    ->AddTextInput("password", "Password", "", 24)
    ->EndDialog("password_reply", "OK", "Cancel");

    pPlayer->SendOnDialogRequest(db.Get());
}

void DoorDialog::HandlePasswordReply(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if(!pPlayer)
        return;

    if(!pPlayer)
        return;

    auto pTileX = packet.Find("tilex"_hash);
    if(!pTileX)
        return;

    auto pTileY = packet.Find("tiley"_hash);
    if(!pTileY)
        return;

    auto pPassword = packet.Find("password"_hash);
    if(!pTileY)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    int32 tileX = 0;
    if(pTileX->GetInt(tileX) != TO_INT_SUCCESS)
        return;

    int32 tileY = 0;
    if(pTileY->GetInt(tileY) != TO_INT_SUCCESS)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if(!pTile)
        return;

    if(pTile->GetFG() != ITEM_ID_PASSWORD_DOOR && pTile->GetFG() != ITEM_ID_HAUNTED_DOOR)
        return;

    if(pPlayer->GetDistToTileInTiles(pTile) > 2)
    {
        pPlayer->SendOnTalkBubble("The door can't hear me say the password from this distance.", false);
        return;
    }

    TileExtra_Door* pTileExtra = pTile->GetExtra<TileExtra_Door>();
    if(!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The door is gone!", false);
        return;
    }

    if(pPassword->valueSize != pTileExtra->id.size() || ToLower(pPassword->GetString().data()) != ToLower(pTileExtra->id))
    {
        pPlayer->SendOnTalkBubble("`4Wrong password!``", false);
        return;
    }

    if(pTileExtra->text.empty())
    {
        pPlayer->SendOnTalkBubble("`2The door opens! But nothing is behind it.``", false);
        return;
    }

    pPlayer->SendOnTalkBubble("`2The door opens!``", false);
    pPlayer->SendOnSetFreezeState(PLAYER_FREEZE_STATE_FROZEN, 0);

    auto targetWorld = Split(pTileExtra->text, ':');
    if(targetWorld.empty())
    {
        targetWorld.push_back(""); 
    }

    if(targetWorld[0].empty())
    {
        targetWorld[0] = pWorld->GetWorlName();
    }

    string targetDoorID = (targetWorld.size() > 1) ? targetWorld[1] : "";
    pPlayer->SetTargetJoinWorld(targetWorld[0], targetDoorID);
}
