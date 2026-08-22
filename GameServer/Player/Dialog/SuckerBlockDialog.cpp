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
    if (!pItem || pItem->type != ITEM_TYPE_SUCKER)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    pWorld->GetSuckerBlockManager().IsCorrupted(pTile);

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon(pItem->name + "``", pItem->id, true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y);

    bool isOwner = pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID());

    if (pTileExtra->itemID == ITEM_ID_BLANK)
    {
        if (isOwner)
        {
            db.AddTextBox("`6The machine is empty.``")
                .AddItemPicker("selectitem", "`wChoose Item``", "Choose an item to put in the " + pItem->name);
        }
        else
        {
            db.AddTextBox("There is no item selected.");
        }

        db.EndDialog("itemsucker", "", "Close");
        pPlayer->SendOnDialogRequest(db.Get());
        return;
    }

    ItemInfo* pSuckedItemInfo = GetItemInfoManager()->GetItemByID(pTileExtra->itemID);
    if (!pSuckedItemInfo)
        return;

    int32 tileFg = pTile->GetFG();

    db.AddSpacer().AddLabelWithIcon("`2" + pSuckedItemInfo->name + "``", pSuckedItemInfo->id);

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
        {
            db.AddButton("clearitem", "Change Item");
        }
        else if (pTileExtra->count == 1)
        {
            db.AddButton("retrieveitem", "Retrieve Item");
        }
        else
        {
            if (pPlayer->GetInventory().GetCountOfItem(pTileExtra->itemID) >= 200)
                db.AddTextBox("You have too many " + pSuckedItemInfo->name + "!");
            else
                db.AddButton("retrieveitem", "Retrieve Items");
        }
    }

    if (tileFg != ITEM_ID_GAIAS_BEACON && tileFg != ITEM_ID_UNSTABLE_TESSERACT)
    {
        if (canPlantOrBuildItem)
        {
            if (pTileExtra->isPlanting == 1)
            {
                if (pSuckedItemInfo->type == ITEM_TYPE_SEED)
                    db.AddTextBox("Planting mode: `5ACTIVE``");
                else
                    db.AddTextBox("Building mode: `5ACTIVE``");
            }
            else
            {
                if (pSuckedItemInfo->type == ITEM_TYPE_SEED)
                {
                    db.AddTextBox("Planting mode: `6DISABLED``");
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
        {
            db.AddTextBox("`6You cannot place this item.``");
        }
    }

    if (isOwner)
    {
        db.AddCheckBox("chk_enablesucking", "Enable Collection", pTileExtra->isSucking == 1)
            .EndDialog("itemsucker", "Update", "Close");
    }
    else
    {
        if (tileFg != ITEM_ID_GAIAS_BEACON && tileFg != ITEM_ID_UNSTABLE_TESSERACT)
        {
            db.EndDialog("itemsucker", "", "Close");
        }
        else
            return;
    }

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
    if (!pItem || pItem->type != ITEM_TYPE_SUCKER)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    if (!pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
        return;

    int32 maxCapacity = SuckerBlockManager::GetMachineCapacity(pItem->id);

    if (pTileExtra->count >= maxCapacity)
    {
        pPlayer->SendOnTalkBubble(
            "You cannot add more items, the " + pItem->name + " is currently at maximum capacity.", false);
        return;
    }

    ItemInfo* pSuckedItemInfo = GetItemInfoManager()->GetItemByID(pTileExtra->itemID);
    if (!pSuckedItemInfo)
        return;

    int32 playerItemCount = pPlayer->GetInventory().GetCountOfItem(pTileExtra->itemID);
    if (playerItemCount < 1)
    {
        pPlayer->SendOnTalkBubble("You don't have " + pSuckedItemInfo->name + "!", false);
        return;
    }

    int32 availableSpace = maxCapacity - pTileExtra->count;
    int32 defaultAmountToAdd = Min(playerItemCount, availableSpace);

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddSpacer()
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y)
        .AddLabelWithIcon("`2" + pSuckedItemInfo->name + "``", pSuckedItemInfo->id)
        .AddTextBox("You have " + ToString(playerItemCount) + " `2" + pSuckedItemInfo->name + "`` in your backpack.")
        .AddTextBox("How many `2" + pSuckedItemInfo->name + "`` would you like to add?")
        .AddTextInput("itemtoadd", "Amount:", ToString(defaultAmountToAdd), 20)
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
    if (!pItem || pItem->type != ITEM_TYPE_SUCKER)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    if (!pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
        return;

    if (pTileExtra->count < 1)
    {
        pPlayer->SendOnTalkBubble("You don't have any items to retrieve.", false);
        return;
    }

    ItemInfo* pSuckedItemInfo = GetItemInfoManager()->GetItemByID(pTileExtra->itemID);
    if (!pSuckedItemInfo)
        return;

    int32 playerHoldCount = pPlayer->GetInventory().GetCountOfItem(pSuckedItemInfo->id);
    int32 remainingSpace = pSuckedItemInfo->maxCanHold - playerHoldCount;

    if (remainingSpace <= 0 || !pPlayer->GetInventory().HaveRoomForItem(pSuckedItemInfo->id, 1))
    {
        pPlayer->SendOnTalkBubble("You don't have any space left in your backpack for this.", false);
        return;
    }

    int32 defaultRetrieveAmount = Min(pTileExtra->count, remainingSpace);

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddSpacer()
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y)
        .AddLabelWithIcon("`2" + pSuckedItemInfo->name + "``", pSuckedItemInfo->id)
        .AddTextBox("How many `2" + pSuckedItemInfo->name + "`` would you like to remove?")
        .AddTextInput("itemtoremove", "Amount:", ToString(defaultRetrieveAmount), 20)
        .EndDialog("itemremovedfromsucker", "Retrieve", "Close");

    pPlayer->SendOnDialogRequest(db.Get());
}

void SuckerBlockDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pTileX = packet.Find("tilex"_hash);
    auto pTileY = packet.Find("tiley"_hash);
    if (!pTileX || !pTileY)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    int32 tileX = 0, tileY = 0;
    if (pTileX->GetInt(tileX) != TO_INT_SUCCESS || pTileY->GetInt(tileY) != TO_INT_SUCCESS)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if (!pTile)
        return;

    TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? It's gone!", false);
        return;
    }

    SuckerBlockManager& suckerMgr = pWorld->GetSuckerBlockManager();
    bool tileNeedsUpdate = false;
    auto pButtonClicked = packet.Find("buttonClicked"_hash);

    int32 tileFG = pTile->GetFG();

    if (pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
    {
        if (auto pEnableSucking = packet.Find("chk_enablesucking"_hash))
        {
            bool val = false;
            if (pEnableSucking->GetBool(val) == TO_INT_SUCCESS)
            {
                uint8 newSuckingState = val ? 1 : 0;
                if (pTileExtra->isSucking != newSuckingState)
                {
                    pTileExtra->isSucking = newSuckingState;
                    tileNeedsUpdate = true;
                }
            }
        }

        if (auto pItemID = packet.Find("selectitem"_hash))
        {
            uint32 itemID = 0;
            if (pItemID->GetUInt(itemID) == TO_INT_SUCCESS)
            {
                ItemInfo* pSelectedItem = GetItemInfoManager()->GetItemByID(itemID);
                if (pSelectedItem)
                {
                    if (pPlayer->GetInventory().GetCountOfItem(pSelectedItem->id) < 1)
                    {
                        pPlayer->SendOnTalkBubble("You don't have that.", false);
                        return;
                    }

                    suckerMgr.IsCorrupted(pTile);
                    if (pTileExtra->itemID == ITEM_ID_BLANK)
                    {
                        if (SuckerBlockManager::IsAllowedItemInMachine(pSelectedItem->id, tileFG))
                        {
                            suckerMgr.ChangeSuckerItem(pTile, pTileExtra->itemID, pSelectedItem->id);

                            pTileExtra->itemID = pSelectedItem->id;
                            pTileExtra->count = 0;
                            pTileExtra->isSucking = 1;
                            pTileExtra->isPlanting = 0;

                            tileNeedsUpdate = true;
                        }
                        else
                        {
                            if (tileFG == ITEM_ID_GAIAS_BEACON && pSelectedItem->type != ITEM_TYPE_SEED)
                            {
                                pPlayer->SendOnTalkBubble("You can only store seeds in this machine.", false);
                                return;
                            }

                            if (tileFG == ITEM_ID_UNSTABLE_TESSERACT && pSelectedItem->type != ITEM_TYPE_SEED)
                            {
                                pPlayer->SendOnTalkBubble("You cannot store seeds in this machine.", false);
                                return;
                            }

                            pPlayer->SendOnTalkBubble("This item is not compatible.", false);
                            return;
                        }
                    }
                    else
                    {
                        pPlayer->SendOnTalkBubble("You cannot select an item.", false);
                    }
                }
            }
        }

        if (pButtonClicked)
        {
            std::string_view buttonClicked = pButtonClicked->GetStringView();

            if (buttonClicked == "clearitem")
            {
                if (pTileExtra->count != 0)
                {
                    pPlayer->SendOnTalkBubble("Empty the machine first.", false);
                    return;
                }

                suckerMgr.TogglePlanting(pPlayer, pTile);
                suckerMgr.ChangeSuckerItem(pTile, pTileExtra->itemID, ITEM_ID_BLANK);

                pTileExtra->itemID = 0;
                pTileExtra->count = 0;
                pTileExtra->isSucking = 1;
                pTileExtra->isPlanting = 0;

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

    if (tileFG != ITEM_ID_GAIAS_BEACON && tileFG != ITEM_ID_UNSTABLE_TESSERACT)
    {
        if (pButtonClicked && pButtonClicked->GetStringView() == "getplantationdevice")
        {
            suckerMgr.GiveRemoteToPlayer(pPlayer);
            return;
        }
    }

    if (tileNeedsUpdate)
    {
        pWorld->SendTileUpdate(pTile);
    }
}

void SuckerBlockDialog::HandleAddItem(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pTileX = packet.Find("tilex"_hash);
    auto pTileY = packet.Find("tiley"_hash);
    if (!pTileX || !pTileY)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    int32 tileX = 0, tileY = 0;
    if (pTileX->GetInt(tileX) != TO_INT_SUCCESS || pTileY->GetInt(tileY) != TO_INT_SUCCESS)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if (!pTile)
        return;

    TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? It's gone!", false);
        return;
    }

    if (!pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
        return;

    ItemInfo* pSuckedItem = GetItemInfoManager()->GetItemByID(pTileExtra->itemID);
    if (!pSuckedItem || pSuckedItem->HasFlag(ITEM_FLAG_UNTRADEABLE))
        return;

    int32 itemToAdd = 0;
    auto pItemToAdd = packet.Find("itemtoadd"_hash);
    if (!pItemToAdd || pItemToAdd->GetInt(itemToAdd) != TO_INT_SUCCESS || itemToAdd <= 0)
        return;

    int32 playerInventoryCount = pPlayer->GetInventory().GetCountOfItem(pSuckedItem->id);
    if (playerInventoryCount < itemToAdd)
    {
        pPlayer->SendOnTalkBubble("You don't have " + ToString(itemToAdd) + " " + pSuckedItem->name + "!", false);
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
    auto pTileY = packet.Find("tiley"_hash);
    if (!pTileX || !pTileY)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    int32 tileX = 0, tileY = 0;
    if (pTileX->GetInt(tileX) != TO_INT_SUCCESS || pTileY->GetInt(tileY) != TO_INT_SUCCESS)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if (!pTile)
        return;

    TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? It's gone!", false);
        return;
    }

    if (!pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
        return;

    ItemInfo* pSuckedItem = GetItemInfoManager()->GetItemByID(pTileExtra->itemID);
    if (!pSuckedItem || pSuckedItem->HasFlag(ITEM_FLAG_UNTRADEABLE))
        return;

    int32 itemToRemove = 0;
    auto pItemToRemove = packet.Find("itemtoremove"_hash);
    if (!pItemToRemove || pItemToRemove->GetInt(itemToRemove) != TO_INT_SUCCESS || itemToRemove <= 0)
        return;

    if (pTileExtra->count < itemToRemove)
    {
        pPlayer->SendOnTalkBubble("You don't have that many items in the machine.", false);
        return;
    }

    if (!pPlayer->GetInventory().HaveRoomForItem(pSuckedItem->id, itemToRemove))
    {
        pPlayer->SendOnTalkBubble("You don't have enough space in your backpack for this.", false);
        return;
    }

    pTileExtra->count -= itemToRemove;
    pPlayer->ModifyInventoryItem(pSuckedItem->id, itemToRemove);
    pWorld->SendTileUpdate(pTile);
}