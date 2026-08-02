#include "MannequinDialog.h"
#include "../../World/WorldManager.h"
#include "../GamePlayer.h"
#include "Item/ItemInfoManager.h"
#include "Utils/DialogBuilder.h"

void MannequinDialog::Request(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile)
        return;

    TileExtra_Mannequin* pTileExtra = pTile->GetExtra<TileExtra_Mannequin>();
    if (!pTileExtra)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem)
        return;

    if (pItem->type != ITEM_TYPE_MANNEQUIN)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        return;

    if (pTileExtra->text.find("__&%@PL@%&__") != string::npos)
    {
        pTileExtra->text = "";
    }

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon("`wEdit " + pItem->name + "``", pItem->id, true)
        .AddTextBox("To dress, select a clothing item then use on the mannequin. To remove clothes, punch it or select "
                    "which item "
                    "to remove.<CR><CR>It will go into your backpack if you have room.");

    bool hasAnyClothing = false;

    for (int32 i = 0; i < 9; ++i)
    {
        if (pTileExtra->clothes[i] == ITEM_ID_BLANK)
            continue;

        ItemInfo* pCloth = GetItemInfoManager()->GetItemByID(pTileExtra->clothes[i]);
        if (!pCloth)
            continue;

        db.AddCheckBox("checkbox", pCloth->name, false);
        hasAnyClothing = true;
    }

    if (hasAnyClothing)
    {
        db.AddSpacer()
            .AddButton("clear_selected", "`4Remove Selected Items``")
            .AddButton("clear", "`4Remove All Items``");
    }

    Vector2Int& vTilePos = pTile->GetPos();
    db.AddTextBox("<CR><CR>What would you like to write on its sign?``")
        .AddTextInput("sign_text", "", pTileExtra->text, 128)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y)
        .EndDialog("mannequin_edit", "OK", "Cancel");

    pPlayer->SendOnDialogRequest(db.Get());
}

void MannequinDialog::RequestPutItem(GamePlayer* pPlayer, TileInfo* pTile, int32 itemID, bool fromDialog)
{
    if (!pPlayer || !pTile)
        return;

    TileExtra_Mannequin* pTileExtra = pTile->GetExtra<TileExtra_Mannequin>();
    if (!pTileExtra)
        return;

    if (pPlayer->GetDistToTileInTiles(pTile) >= 4)
    {
        pPlayer->SendOnTalkBubble("`5[`2I'll need to get closer``]``", false);
        return;
    }

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem)
        return;

    if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
    {
        pPlayer->SendOnTalkBubble("`5[`2That " + pItem->name + " isn't yours!``]``", false);
        return;
    }

    if (!pWorld->GetTileManager()->IsTileLockedWithLock(pTile))
    {
        pPlayer->SendOnTalkBubble("`5[`2It's not safe to use an unlocked " + pItem->name + " !``]``", false);
        return;
    }

    if (pTile->GetBG() == ITEM_ID_DARK_CAVE_BACKGROUND)
    {
        if (!pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
        {
            pPlayer->SendOnTalkBubble("`5[`2It's too dark to use this " + pItem->name + " !``]``", false);
            return;
        }
    }

    ItemInfo* pCloth = GetItemInfoManager()->GetItemByID(itemID);
    if (!pCloth)
        return;

    if (pCloth->type != ITEM_TYPE_CLOTHES)
    {
        pPlayer->SendOnTalkBubble(
            "`5[`2That would just be weird.  Try putting clothes on your " + pItem->name + " instead!``]``", false);
        return;
    }

    if (pItem->id == ITEM_ID_SNOWTOPIAN && (pCloth->bodyPart == BODY_PART_PANT || pCloth->bodyPart == BODY_PART_SHOE))
    {
        static string partNames[9] = {"Hat",    "Shirts",   "Pants", "Shoes",      "Face Items",
                                      "Gloves", "Backpack", "Hairs", "Chest Items"};

        pPlayer->SendOnTalkBubble("`5[`2A " + pItem->name + " cannot wear " + partNames[pCloth->bodyPart] + "!``]``",
                                  false);
        return;
    }

    if (pCloth->HasFlag(ITEM_FLAG_UNTRADEABLE))
    {
        pPlayer->SendOnTalkBubble("`5[`2That item is just too valuable to part with``]``", false);
        return;
    }

    if (pPlayer->GetInventory().GetCountOfItem(pCloth->id) < 1)
    {
        pPlayer->SendOnTalkBubble("`5[`2You don't have that item anymore``]``", false);
        return;
    }

    int32 currentEquipped = pTileExtra->clothes[pCloth->bodyPart];
    if (currentEquipped == pCloth->id)
    {
        pPlayer->SendOnTalkBubble("`5[`2You giggle as you swap two identical items``]``", false);
        return;
    }

    if (fromDialog)
    {
        pTileExtra->SetCloth(pCloth->bodyPart, pCloth->id);

        if (pCloth->bodyPart == BODY_PART_HAIR)
        {
            // todo hair color
        }

        pTileExtra->charFlags = pPlayer->GetCharData().charState;
        pTileExtra->char2Flags = pPlayer->GetCharData().char2State;

        pPlayer->ModifyInventoryItem(pCloth->id, -1);
        pWorld->SendPlayPositionedToAll(pPlayer, "change_clothes.wav");

        if (currentEquipped != ITEM_ID_BLANK)
        {
            pPlayer->ModifyInventoryItem(currentEquipped, 1);
            // todo drop
        }

        pWorld->SendTileUpdate(pTile);
    }
    else
    {
        Vector2Int& vTilePos = pTile->GetPos();

        DialogBuilder db;
        db.SetDefaultColor('o')
            .AddLabelWithIcon("`w" + pCloth->name, pItem->id, true)
            .AddTextBox("Do you really want to put your " + pCloth->name + " on the " + pItem->name + "?")
            .EmbedData("put", pCloth->id)
            .EmbedData("tilex", vTilePos.x)
            .EmbedData("tiley", vTilePos.y)
            .EndDialog("mannequin_edit", "Yes", "No");

        pPlayer->SendOnDialogRequest(db.Get());
    }
}

bool MannequinDialog::RequestRemoveItem(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile)
        return false;

    TileExtra_Mannequin* pTileExtra = pTile->GetExtra<TileExtra_Mannequin>();
    if (!pTileExtra)
        return false;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return false;

    if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        return false;

    int32 clothIndex = -1;

    for (int32 i = 0; i < 9; ++i)
    {
        if (pTileExtra->clothes[i] != ITEM_ID_BLANK)
        {
            clothIndex = i;
            break;
        }
    }

    if (clothIndex == -1)
        return false;

    if (pPlayer->GetInventory().GetFitItemCount(pTileExtra->clothes[clothIndex]) < 1)
    {
        pPlayer->SendOnTalkBubble("You don't have room for the item!", false);
        return true;
    }

    pTileExtra->SetCloth(clothIndex, ITEM_ID_BLANK);
    pPlayer->ModifyInventoryItem(pTileExtra->clothes[clothIndex], 1);

    pWorld->SendPlayPositionedToAll(pPlayer, "change_clothes.wav");
    pWorld->SendTileUpdate(pTile);
    return true;
}

void MannequinDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
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

    TileExtra_Mannequin* pTileExtra = pTile->GetExtra<TileExtra_Mannequin>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The mannequin is gone!", false);
        return;
    }

    if (auto pPut = packet.Find("put"_hash))
    {
        int32 itemID = 0;
        if (pPut->GetInt(itemID) != TO_INT_SUCCESS)
            return;

        RequestPutItem(pPlayer, pTile, itemID, true);
        return;
    }

    if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        return;

    bool tileNeedsUpdate = false;

    if (auto pButtonClicked = packet.Find("buttonClicked"_hash))
    {
        const std::string_view buttonClicked = pButtonClicked->GetStringView();

        if (buttonClicked == "clear" || buttonClicked == "clear_selected")
        {
            std::string_view prefix = "checkbox";
            int32 clothIndex = 0;

            for (uint8 i = 0; i < packet.count; ++i)
            {
                if (packet.fields[i].keySize == 0)
                    continue;

                std::string_view key = packet.fields[i].GetKeyStringView();
                if (key.size() != prefix.size() || key != prefix)
                    continue;

                while (clothIndex < 9 && pTileExtra->clothes[clothIndex] == 0)
                    clothIndex++;

                if (clothIndex >= 9)
                    break;

                bool isChecked;
                if (packet.fields[i].GetBool(isChecked) != TO_INT_SUCCESS)
                    continue;

                if (isChecked || buttonClicked == "clear")
                {
                    ItemInfo* pCloth = GetItemInfoManager()->GetItemByID(pTileExtra->clothes[clothIndex]);
                    if (!pCloth)
                    {
                        clothIndex++;
                        continue;
                    }

                    bool invHasItem = pPlayer->GetInventory().GetCountOfItem(pCloth->id) > 0;
                    bool canItemFit = pPlayer->GetInventory().GetFitItemCount(pCloth->id) > 0;

                    if (invHasItem && !canItemFit)
                    {
                        pPlayer->SendOnTalkBubble(
                            "Hmm, I'll need to drop the " + pCloth->name +
                                " items I already have before I can get the ones in mannequin, they won't fit.",
                            false);
                    }

                    if (!invHasItem && !canItemFit)
                    {
                        pPlayer->SendOnTalkBubble("I don't have enough room in my backpack to get `5" + pCloth->name +
                                                      " ``from the mannequin!",
                                                  false);
                    }

                    if (canItemFit)
                    {
                        pPlayer->ModifyInventoryItem(pCloth->id, 1);
                        tileNeedsUpdate = true;
                        pTileExtra->SetCloth(clothIndex, ITEM_ID_BLANK);
                    }
                }

                clothIndex++;
            }
        }
    }

    if (tileNeedsUpdate)
    {
        pWorld->SendTileUpdate(pTile);
    }

    if (auto pSignText = packet.Find("sign_text"_hash))
    {
        if (pSignText->valueSize > 258)
            return;

        const std::string_view signText = pSignText->GetStringView();

        if (signText.find("__&%@PL@%&__") != string::npos)
            return;

        if (signText != pTileExtra->text)
        {
            pTileExtra->text = signText;
            pWorld->SendTileUpdate(pTile);
        }
    }
}