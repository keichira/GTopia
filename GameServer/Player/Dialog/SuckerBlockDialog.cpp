#include "SuckerBlockDialog.h"
#include "../../World/WorldManager.h"
#include "../GamePlayer.h"
#include "Item/ItemInfoManager.h"
#include "Item/SuckerBlockManager.h"
#include "Utils/DialogBuilder.h"

void SuckerBlockDialog::Request(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile)
        return;

    TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>();
    if (!pTileExtra)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem)
        return;

    if (pItem->type != ITEM_TYPE_SUCKER)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon(pItem->name + "``", pItem->id, true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y);

    SuckerBlockManager::IsCorrupted(pTileExtra);

    bool isOwner = pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID());

    if (pTileExtra->itemID == ITEM_ID_BLANK)
    {
        if (isOwner)
        {
            db.AddTextBox("`6The machine is empty.``")
                .AddItemPicker("selectitem", "`wChoose Item``", "Choose an item to put in the " + pItem->name);
        }
        else
            db.AddTextBox("There is no item selected.");

        db.EndDialog("itemsucker", "", "Close");
        pPlayer->SendOnDialogRequest(db.Get());
        return;
    }

    ItemInfo* pSuckedItemInfo = GetItemInfoManager()->GetItemByID(pTileExtra->itemID);
    if (!pSuckedItemInfo)
        return;

    db.AddLabelWithIcon("`2" + pSuckedItemInfo->name + "``", pSuckedItemInfo->id);

    if (isOwner)
    {
        if (pTileExtra->count == 0)
        {
            db.AddTextBox("`6The machine is currently empty!``");
        }
        else
        {
            db.AddTextBox("Machine contains " + ToString(pTileExtra->count) + " `6" + pSuckedItemInfo->name + "``");
        }
    }

    bool canPlantOrBuildItem = SuckerBlockManager::IsAllowedToBuildOrPlantItem(pTileExtra->itemID);

    if (isOwner)
    {
        if (SuckerBlockManager::GetMachineCapacity(pItem->id) == pTileExtra->count)
        {
            db.AddTextBox("`4The machine is at maximum capacity!``");
        }
        else
        {
            if (pPlayer->GetInventory().GetCountOfItem(pTileExtra->itemID) > 0)
                db.AddButton("additem", "Add Items to the machine");
        }

        if (pTileExtra->count == 0)
            db.AddButton("clearitem", "Change Item");
        else if (pTileExtra->count == 1)
            db.AddButton("retrieveitem", "Retrieve Item");
        else
        {
            if (pPlayer->GetInventory().GetCountOfItem(pTileExtra->itemID) == 200)
                db.AddTextBox("You have too many " + pSuckedItemInfo->name + "!");
            else
                db.AddButton("retrieveitem", "Retrieve Items");
        }
    }

    if (canPlantOrBuildItem)
    {
        if (pTileExtra->isPlanting == 1)
        {
            if (pSuckedItemInfo->type == ITEM_TYPE_SEED)
            {
                db.AddTextBox("Planting mode: `5ACTIVE``");
            }
            else
            {
                db.AddTextBox("Building mode: `5ACTIVE``");
            }
        }
        else
        {
            if (pSuckedItemInfo->type == ITEM_TYPE_SEED)
            {
                db.AddTextBox("Planting mode: `6DISABLED`");

                if (isOwner)
                    db.AddTextBox("Punch to activate planting mode.");
            }
            else
            {
                db.AddTextBox("Building mode: `6DISABLED``");

                if (isOwner)
                    db.AddTextBox("Punch to activate building mode.");
            }
        }

        if (pPlayer->GetInventory().GetCountOfItem(ITEM_ID_MAGPLANT_5000_REMOTE) < 1)
            db.AddButton("getplantationdevice", "Get Remote");
    }
    else
        db.AddTextBox("`6You cannot place this item.``");

    if (isOwner)
    {
        db.AddCheckBox("chk_enablesucking", "Enable Collection", pTileExtra->isSucking == 1 ? true : false)
            .EndDialog("itemsucker", "Update", "Close");
    }
    else
        db.EndDialog("itemsucker", "", "Close");

    pPlayer->SendOnDialogRequest(db.Get());
}

void SuckerBlockDialog::RequestAddItem(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile)
        return;

    TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>();
    if (!pTileExtra)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem)
        return;

    if (pItem->type != ITEM_TYPE_SUCKER)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    if (!pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
        return;

    uint8 amountToAdd = pPlayer->GetInventory().GetCountOfItem(pTileExtra->itemID);
    int32 maxCapacity = SuckerBlockManager::GetMachineCapacity(pItem->id);

    if (amountToAdd > (maxCapacity - amountToAdd))
        amountToAdd = (maxCapacity - amountToAdd);

    if (pTileExtra->count >= maxCapacity)
    {
        pPlayer->SendOnTalkBubble(
            "You cannot add more items, the " + pItem->name + " is currently at maximum capacity.", false);
        return;
    }

    ItemInfo* pSuckedItemInfo = GetItemInfoManager()->GetItemByID(pTileExtra->itemID);
    if (!pSuckedItemInfo)
        return;

    if (amountToAdd == 0)
    {
        pPlayer->SendOnTalkBubble("You don't have " + pSuckedItemInfo->name, false);
        return;
    }

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddSpacer()
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y)
        .AddLabelWithIcon("`2" + pSuckedItemInfo->name + "``", pSuckedItemInfo->id)
        .AddTextBox("You have " + ToString(pPlayer->GetInventory().GetCountOfItem(pTileExtra->itemID)) + " `2" +
                    pSuckedItemInfo->name + "`` in your backpack.")
        .AddTextBox("How many `2" + pSuckedItemInfo->name + "`` would you like to add?")
        .AddTextInput("itemtoadd", "Amount:", ToString(amountToAdd), 20)
        .EndDialog("itemaddedtosucker", "Add", "Close");

    pPlayer->SendOnDialogRequest(db.Get());
}

void SuckerBlockDialog::RequestRetrieveItem(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile)
        return;

    TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>();
    if (!pTileExtra)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem)
        return;

    if (pItem->type != ITEM_TYPE_SUCKER)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    if (!pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
        return;

    ItemInfo* pSuckedItemInfo = GetItemInfoManager()->GetItemByID(pTileExtra->itemID);
    if (!pSuckedItemInfo)
        return;

    int32 remainingSpace = pSuckedItemInfo->maxCanHold - pPlayer->GetInventory().GetCountOfItem(pSuckedItemInfo->id);
    if (!pPlayer->GetInventory().HaveRoomForItem(pSuckedItemInfo->id, remainingSpace))
    {
        pPlayer->SendOnTalkBubble("You dont have any space left in your backpack for this", false);
        return;
    }

    if (pTileExtra->count < 1)
    {
        pPlayer->SendOnTalkBubble("You dont have any items to retrieve.", false);
        return;
    }

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddSpacer()
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y)
        .AddLabelWithIcon("`2" + pSuckedItemInfo->name + "``", pSuckedItemInfo->id)
        .AddTextBox("How many `2" + pSuckedItemInfo->name + "`` would you like to remove?")
        .AddTextInput("itemtoremove", "Amount:", ToString(Min(pTileExtra->count, remainingSpace)), 20)
        .EndDialog("itemremovedfromsucker", "Retrieve", "Close");

    pPlayer->SendOnDialogRequest(db.Get());
}

void SuckerBlockDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pTileX = packet.Find("tilex"_hash);
    if (!pTileX)
        return;

    auto pTileY = packet.Find("tiley"_hash);
    if (!pTileY)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    int32 tileX = 0;
    if (pTileX->GetInt(tileX) != TO_INT_SUCCESS)
        return;

    int32 tileY = 0;
    if (pTileY->GetInt(tileY) != TO_INT_SUCCESS)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if (!pTile)
        return;

    TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? It's is gone!", false);
        return;
    }

    bool tileNeedsUpdate = false;
    auto pButtonClicked = packet.Find("buttonClicked"_hash);

    if (pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
    {
        if (auto pEnableSucking = packet.Find("chk_enablesucking"_hash))
        {
            bool val;
            if (pEnableSucking->GetBool(val) != TO_INT_SUCCESS)
                return;

            if (pTileExtra->isSucking != (val ? 1 : 0))
            {
                pTileExtra->isSucking = val ? 1 : 0;
                tileNeedsUpdate = true;
            }
        }

        if (auto pItemID = packet.Find("selectitem"_hash))
        {
            uint32 itemID = 0;
            if (pItemID->GetUInt(itemID) != TO_INT_SUCCESS)
                return;

            ItemInfo* pSelectedItem = GetItemInfoManager()->GetItemByID(itemID);
            if (!pSelectedItem)
                return;

            SuckerBlockManager::IsCorrupted(pTileExtra);
            if (pTileExtra->itemID == ITEM_ID_BLANK)
            {
                if (SuckerBlockManager::IsAllowedItemInMachine(pSelectedItem->id, pTile->GetFG()))
                {
                    pTileExtra->itemID = pSelectedItem->id;
                    pTileExtra->count = 0;
                    pTileExtra->isSucking = 1;
                    pTileExtra->isPlanting = 0;
                    tileNeedsUpdate = true;
                }
                else
                {
                    pPlayer->SendOnTalkBubble("This item is not compatible.", false);
                    return;
                }
            }
            else
            {
                pPlayer->SendOnTalkBubble("You cannot Select an item", false);
            }
        }

        if (pButtonClicked)
        {
            std::string_view buttonClicked = pButtonClicked->GetStringView();

            if (buttonClicked == "clearitem")
            {
                pTileExtra->itemID = 0;
                pTileExtra->count = 0;
                pTileExtra->isSucking = 1;
                pTileExtra->isPlanting = 0;

                TileInfo* pActivePlanterTile = pWorld->GetSuckerBlockManager().GetActivePlanter();
                if (!pActivePlanterTile || pActivePlanterTile == pTile)
                {
                    pWorld->GetSuckerBlockManager().TogglePlanting(pPlayer, pTile);
                }

                pWorld->SendTileUpdate(pTile);
                Request(pPlayer, pTile);
                return;
            }

            if (buttonClicked == "additem")
            {
                RequestAddItem(pPlayer, pTile);
                return;
            }

            if (buttonClicked == "retrieveitem")
            {
                RequestRetrieveItem(pPlayer, pTile);
                return;
            }
        }
    }

    if (pButtonClicked && pButtonClicked->GetStringView() == "getplantationdevice")
    {
        pWorld->GetSuckerBlockManager().GiveRemoteToPlayer(pPlayer);
        return;
    }

    if (tileNeedsUpdate)
        pWorld->SendTileUpdate(pTile);
}

void SuckerBlockDialog::HandleAddItem(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pTileX = packet.Find("tilex"_hash);
    if (!pTileX)
        return;

    auto pTileY = packet.Find("tiley"_hash);
    if (!pTileY)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    int32 tileX = 0;
    if (pTileX->GetInt(tileX) != TO_INT_SUCCESS)
        return;

    int32 tileY = 0;
    if (pTileY->GetInt(tileY) != TO_INT_SUCCESS)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if (!pTile)
        return;

    TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? It's is gone!", false);
        return;
    }

    if (!pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
        return;

    ItemInfo* pSuckedItem = GetItemInfoManager()->GetItemByID(pTileExtra->itemID);
    if (!pSuckedItem || pSuckedItem->HasFlag(ITEM_FLAG_UNTRADEABLE))
        return;

    int32 itemToAdd = 0;
    auto pItemToAdd = packet.Find("itemtoadd"_hash);
    if (!pItemToAdd || pItemToAdd->GetInt(itemToAdd) != TO_INT_SUCCESS)
        return;

    if (itemToAdd < 0)
        itemToAdd = 0;
    if (itemToAdd > pSuckedItem->maxCanHold)
        itemToAdd = pSuckedItem->maxCanHold;

    if (pPlayer->GetInventory().GetCountOfItem(pSuckedItem->id) < itemToAdd)
    {
        pPlayer->SendOnTalkBubble("You don't have " + pSuckedItem->name + "!", false);
        return;
    }

    if (pTileExtra->count + itemToAdd > SuckerBlockManager::GetMachineCapacity(pTile->GetFG()))
    {
        if (ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG()))
        {
            pPlayer->SendOnTalkBubble("You cannot add this amount, it will overload the " + pItem->name + "!", false);
        }
        return;
    }

    pTileExtra->count += itemToAdd;
    pPlayer->ModifyInventoryItem(pSuckedItem->id, -itemToAdd);
    pWorld->SendTileUpdate(pTile);
}

void SuckerBlockDialog::HandleRetrieveItem(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pTileX = packet.Find("tilex"_hash);
    if (!pTileX)
        return;

    auto pTileY = packet.Find("tiley"_hash);
    if (!pTileY)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    int32 tileX = 0;
    if (pTileX->GetInt(tileX) != TO_INT_SUCCESS)
        return;

    int32 tileY = 0;
    if (pTileY->GetInt(tileY) != TO_INT_SUCCESS)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if (!pTile)
        return;

    TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? It's is gone!", false);
        return;
    }

    if (!pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
        return;

    ItemInfo* pSuckedItem = GetItemInfoManager()->GetItemByID(pTileExtra->itemID);
    if (!pSuckedItem || pSuckedItem->HasFlag(ITEM_FLAG_UNTRADEABLE))
        return;

    int32 itemToRemove = 0;
    auto pItemToRemove = packet.Find("itemtoremove"_hash);
    if (!pItemToRemove || pItemToRemove->GetInt(itemToRemove) != TO_INT_SUCCESS)
        return;

    if (itemToRemove < 0)
        itemToRemove = 0;
    if (itemToRemove > pSuckedItem->maxCanHold)
        itemToRemove = pSuckedItem->maxCanHold;

    if (!pPlayer->GetInventory().HaveRoomForItem(pSuckedItem->id, itemToRemove))
    {
        pPlayer->SendOnTalkBubble("You don't have enough space in your backpack for this.", false);
        return;
    }

    if (pTileExtra->count < itemToRemove)
    {
        pPlayer->SendOnTalkBubble("You are removing what you dont have.", false);
        return;
    }

    pTileExtra->count -= itemToRemove;
    pPlayer->ModifyInventoryItem(pSuckedItem->id, itemToRemove);
    pWorld->SendTileUpdate(pTile);
}
