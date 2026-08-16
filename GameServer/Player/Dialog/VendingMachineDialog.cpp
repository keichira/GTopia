#include "VendingMachineDialog.h"
#include "../../World/WorldManager.h"
#include "../GamePlayer.h"
#include "Item/ItemInfoManager.h"
#include "Utils/DialogBuilder.h"

void VendingMachineDialog::Request(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    TileExtra_VendingMachine* pTileExtra = pTile->GetExtra<TileExtra_VendingMachine>();
    if (!pTileExtra)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem || pItem->type != ITEM_TYPE_VENDING)
        return;

    TileInfo* pPlayerCenterTile = pPlayer->GetTilePlayerOnCenter();
    if (!pPlayerCenterTile)
        return;

    if (pPlayerCenterTile != pTile)
    {
        pPlayer->SendOnTalkBubble("Get closer!", false);
        return;
    }

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon("`w" + pItem->name + "``", pItem->id, true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y);

    if (pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
    {
        if (pTileExtra->itemID == ITEM_ID_BLANK || pTileExtra->stock == 0)
        {
            db.AddTextBox("This machine is empty")
                .AddItemPicker("stockitem", "`wPut an item in``", "Choose an item to put in the machine!");
        }
        else
        {
        }
    }
    else
    {
    }
}

void VendingMachineDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet) {}
