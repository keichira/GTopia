#include "LockDialog.h"
#include "../GamePlayer.h"
#include "Utils/DialogBuilder.h"
#include "Item/ItemInfoManager.h"
#include "../../World/WorldManager.h"

void LockDialog::Request(GamePlayer* pPlayer, TileInfo* pTile)
{
    if(!pPlayer || !pTile || pPlayer->GetCurrentWorld() == 0) {
        return;
    }

    TileExtra_Lock* pTileExtra = pTile->GetExtra<TileExtra_Lock>();
    if(!pTileExtra) {
        return;
    }

    World* pWorld = GetWorldManager()->GetWorldByID(pPlayer->GetCurrentWorld());
    if(!pWorld) {
        return;
    }

    if(!pWorld->PlayerHasAccessOnTile(pPlayer, pTile)) {
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetDisplayedItem());
    if(!pItem || pItem->type != ITEM_TYPE_LOCK) {
        return;
    }

    DialogBuilder db;
    db.SetDefaultColor('o')
    ->AddLabelWithIcon("`wEdit " + pItem->name, pItem->id, true)
    ->EmbedData("tilex", pTile->GetPos().x)
    ->EmbedData("tiley", pTile->GetPos().y);
    

    if(!IsWorldLock(pItem->id)) {
        bool ignoreAir = pTileExtra->HasFlag(TILE_EXTRA_LOCK_FLAG_IGNORE_AIR);
        db.AddCheckBox("checkbox_ignore", "Ignore empty air", ignoreAir)
        ->AddButton("recalcLock", "`wRe-apply lock``");
    }

    db.EndDialog("lock_edit", "OK", "Cancel");
    pPlayer->SendOnDialogRequest(db.Get());
}

void LockDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    if(pPlayer->GetCurrentWorld() == 0) {
        return;
    }

    auto pTileX = packet.Find(CompileTimeHashString("tilex"));
    auto pTileY = packet.Find(CompileTimeHashString("tiley"));

    if(!pTileX || !pTileY) {
        return;
    }

    int32 tileX = 0;
    if(ToInt(string(pTileX->value, pTileX->size), tileX) != TO_INT_SUCCESS) {
        return;
    }

    int32 tileY = 0;
    if(ToInt(string(pTileY->value, pTileY->size), tileY) != TO_INT_SUCCESS) {
        return;
    }

    World* pWorld = GetWorldManager()->GetWorldByID(pPlayer->GetCurrentWorld());
    if(!pWorld) {
        return;
    }

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if(!pTile) {
        pPlayer->SendOnTalkBubble("I was looking at a lock but now it's gone.  Magic is real!", false);
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if(pItem->type != ITEM_TYPE_LOCK) {
        return;
    }

    TileExtra_Lock* pTileExtra = pTile->GetExtra<TileExtra_Lock>();
    if(!pTileExtra) {
        return;
    }

    if(!pTileExtra->HasAccess(pPlayer->GetUserID())) {
        return;
    }

    if(pTileExtra->ownerID != pPlayer->GetUserID()) { // admin
        /**
         * remove acc
         */
        return;
    }

    if(IsWorldLock(pItem->id)) {

    }
    else {
        auto pIgnoreAir = packet.Find(CompileTimeHashString("checkbox_ignore"));
        if(pIgnoreAir) {
            int32 ignoreAir = pTileExtra->HasFlag(TILE_EXTRA_LOCK_FLAG_IGNORE_AIR);
            if(ToInt(string(pIgnoreAir->value, pIgnoreAir->size), ignoreAir) != TO_INT_SUCCESS) {
                return;
            }

            if(ignoreAir == 1) {
                pTileExtra->SetFlag(TILE_EXTRA_LOCK_FLAG_IGNORE_AIR);
            }
            else {
                pTileExtra->RemoveFlag(TILE_EXTRA_LOCK_FLAG_IGNORE_AIR);
            }
        }
    }

    auto pButtonClicked = packet.Find(CompileTimeHashString("buttonClicked"));
    if(pButtonClicked) {
        string buttonClicked(pButtonClicked->value, pButtonClicked->size);

        if(!IsWorldLock(pItem->id) && buttonClicked == "recalcLock") {
            std::vector<TileInfo*> lockedTiles;
            bool lockSuccsess = pWorld->GetTileManager()->ApplyLockTiles(pTile, GetMaxTilesToLock(pItem->id), pTileExtra->HasFlag(TILE_EXTRA_LOCK_FLAG_IGNORE_AIR), lockedTiles);

            if(!lockSuccsess) {
                pPlayer->SendOnTalkBubble("Something went wrong, unable to re-calc lock", true);
                return;
            }
            else {
                pWorld->SendLockPacketToAll(pPlayer->GetUserID(), pItem->id, lockedTiles, pTile);
            }
        }
    }
}
