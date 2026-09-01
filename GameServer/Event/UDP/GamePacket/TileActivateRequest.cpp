#include "TileActivateRequest.h"
#include "../../../Dialog/GameDialogs.h"
#include "Math/Math.h"
#include "Utils/GrowUtils.h"

void TileActivateRequest::Execute(GamePlayer* pPlayer, World* pWorld, GameUpdatePacket* pPacket)
{
    if (!pPlayer || !pWorld || !pPacket)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(pPacket->field_11, pPacket->field_12);
    if (!pTile)
        return;

    if (pPlayer->GetDistToTileInTiles(pTile) > 5)
        return;

    uint32 displayedItemID = pTile->GetDisplayedItem();

    if (displayedItemID != ITEM_ID_STEAM_LAUNCHER && displayedItemID != ITEM_ID_ANGRY_ADVENTURE_GORILLA)
    {
        //
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(displayedItemID);
    if (!pItem)
        return;

    Vector2Int& vTilePos = pTile->GetPos();

    if (pTile->IsTileExtraType(TILE_EXTRA_TYPE_DOOR))
    {
        TileExtra_Door* pDoorExtra = pTile->GetExtra<TileExtra_Door>();
        if (!pDoorExtra)
            return;

        if (pDoorExtra->HasFlag(TILE_EXTRA_LOCKED) && !pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        {
            pPlayer->SendOnTalkBubble("The door is locked.", false);
            pPlayer->SendOnZoomCamera(10000.0f);
            pPlayer->SendOnSetFreezeState(PLAYER_FREEZE_STATE_NONE, 0);
            return;
        }

        auto targetWorld = Split(pDoorExtra->text, ':');
        if (targetWorld.empty())
        {
            targetWorld.push_back("");
        }

        if (IsMainDoor(pTile->GetFG()))
        {
            targetWorld[0] = "EXIT";
        }

        if (targetWorld[0].empty())
        {
            targetWorld[0] = pWorld->GetWorlName();
        }
        else
        {
            RemoveGTColorCodes(targetWorld[0]);
        }

        // todo secure check

        if (pTile->GetFG() == ITEM_ID_PASSWORD_DOOR || pTile->GetFG() == ITEM_ID_HAUNTED_DOOR)
        {
            if (pDoorExtra->id.empty())
            {
                pPlayer->SendOnTalkBubble("No password has been set yet!", false);
            }
            else
            {
                DoorDialog::RequestPasswordDoor(pPlayer, pTile, pItem);
            }

            pPlayer->SendOnZoomCamera(10000.0f);
            pPlayer->SendOnSetFreezeState(PLAYER_FREEZE_STATE_NONE, 0);
            return;
        }

        string targetDoorID = (targetWorld.size() > 1) ? targetWorld[1] : "";
        pPlayer->SetTargetJoinWorld(targetWorld[0], targetDoorID);

        return;
    }
}