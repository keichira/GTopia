#include "DonationBoxDialog.h"
#include "../../Server/UserCacheManager.h"
#include "../../World/WorldManager.h"
#include "../GamePlayer.h"
#include "Item/ItemInfoManager.h"
#include "Utils/DialogBuilder.h"
#include "Utils/GrowUtils.h"

void DonationBoxDialog::Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem)
{
    if (!pPlayer || !pTile || !pItem)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    TileExtra_DonaitonBox* pTileExtra = pTile->GetExtra<TileExtra_DonaitonBox>();
    if (!pTileExtra)
        return;

    if (pPlayer->GetDistToTileInTiles(pTile) > 4)
    {
        pPlayer->SendOnTalkBubble("`5[`2I'll need to get closer``]``", false);
        return;
    }

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        ->AddLabelWithIcon(pItem->name, pItem->id, true)
        ->EmbedData("tilex", vTilePos.x)
        ->EmbedData("tiley", vTilePos.y);

    if (pTileExtra->gifts.empty())
    {
        db.AddTextBox("The box is currently empty.");
    }

    if (pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
    {
        if (!pTileExtra->gifts.empty())
        {
            std::vector<int32> userIDs;
            userIDs.reserve(pTileExtra->gifts.size());

            for (auto& gift : pTileExtra->gifts)
            {
                userIDs.push_back(gift.userID);
            }

            GetUserCacheManager()->FetchMetadata(pPlayer->GetNetID(), CACHE_REQ_DONATION_BOX_BLOCK, userIDs,
                                                 {pWorld->GetInstanceID(), vTilePos.x, vTilePos.y});
            return;
        }

        db.AddSpacer()->AddTextInput("min_rarity", "Min Rarity:", ToString(pTileExtra->minRarity), 3);

        bool canGift = true;
        if (pTileExtra->gifts.size() >= 20)
        {
            db.AddTextBox("This box already has `w" + ToString(pTileExtra->gifts.size()) +
                          "`` gifts in it, can't add mroe until you cleam them.");
            canGift = false;
        }

        if (canGift)
        {
            db.AddSpacer()->AddItemPicker("itemid",
                                          "`wGive Gift`` (Min Rarity:`5 " + ToString(pTileExtra->minRarity) + "``)",
                                          "Choose an item to give");
        }

        db.EndDialog("donation_box_edit", "Update", "Cancel");
        pPlayer->SendOnDialogRequest(db.Get());
    }
    else
    {
        if (!pTileExtra->gifts.empty())
        {
            db.AddTextBox("You see `w" + ToString(pTileExtra->gifts.size()) + "`` gifts in the box!");
        }

        bool canGift = true;
        if (pTileExtra->gifts.size() >= 20)
        {
            db.AddTextBox("This box already has `w" + ToString(pTileExtra->gifts.size()) +
                          "`` gifts in it. Try again later.");

            canGift = false;
        }

        uint32 playerGiftCount = pTileExtra->GetCountOfGiftsFromID(pPlayer->GetUserID());
        if (canGift && playerGiftCount > 2)
        {
            db.AddTextBox("You've already crammed `w" + ToString(playerGiftCount) +
                          "`` of your gifts into the box, better wait.");

            canGift = false;
        }

        if (canGift)
        {
            db.AddTextBox("Want to leave a gift for the owner?")
                ->AddItemPicker("itemid", "`wGive Gift`` (Min Rarity:`5" + ToString(pTileExtra->minRarity) + "``)",
                                "Choose an item to give");
        }
    }

    db.EndDialog("donation_box_edit", "", "Cancel");
    pPlayer->SendOnDialogRequest(db.Get());
}

void DonationBoxDialog::HandleFromCache(GamePlayer* pPlayer, uint32 worldInstanceID, int32 tileX, int32 tileY)
{
    if (!pPlayer || pPlayer->GetCurrentWorld() != worldInstanceID)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(worldInstanceID);
    if (!pWorld)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if (!pTile)
        return;

    TileExtra_DonaitonBox* pTileExtra = pTile->GetExtra<TileExtra_DonaitonBox>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The box is gone!.", false);
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (pItem->type != ITEM_TYPE_DONATION_BOX)
        return;

    if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        return;

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        ->AddLabelWithIcon(pItem->name, pItem->id, true)
        ->EmbedData("tilex", vTilePos.x)
        ->EmbedData("tiley", vTilePos.y)
        ->AddTextBox("You have `w" + ToString(pTileExtra->gifts.size()) + "`` gift" +
                     (pTileExtra->gifts.size() > 1 ? "s:" : ":") + "waiting:")
        ->AddSpacer();

    UserCacheManager* pUserMgr = GetUserCacheManager();
    ItemInfoManager* pItemMgr = GetItemInfoManager();

    for (auto& gift : pTileExtra->gifts)
    {
        ItemInfo* pItem = pItemMgr->GetItemByID(gift.itemID);
        if (!pItem)
            continue;

        UserMetadata* pMetaData = pUserMgr->GetMetadata(gift.userID);
        string donationMsg;
        donationMsg.reserve(128);

        donationMsg = pItem->name + " (`w" + ToString(gift.itemCount) + "``) from `w" +
                      (pMetaData ? pMetaData->displayName : ("#" + ToString(gift.userID)));

        if (!gift.message.empty())
        {
            donationMsg += "`5 - \" " + gift.message + " \"";
        }

        db.AddCheckBox("checkbox", donationMsg, false);
    }

    db.AddSpacer()
        ->AddButton("clear_selected", "`4Retrieve Selected Gifts``")
        ->AddButton("clear", "`4Retrieve All Gifts``")
        ->AddTextInput("min_rarity", "Min Rarity:", ToString(pTileExtra->minRarity), 3);

    bool canGift = true;
    if (pTileExtra->gifts.size() >= 20)
    {
        db.AddTextBox("This box already has `w" + ToString(pTileExtra->gifts.size()) +
                      "`` gifts in it, can't add mroe until you cleam them.");
        canGift = false;
    }

    if (canGift)
    {
        db.AddSpacer()->AddItemPicker("itemid",
                                      "`wGive Gift`` (Min Rarity:`5 " + ToString(pTileExtra->minRarity) + "``)",
                                      "Choose an item to give");
    }

    db.EndDialog("donation_box_edit", "Update", "Cancel");
    pPlayer->SendOnDialogRequest(db.Get());
}

void DonationBoxDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
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

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem || pItem->type != ITEM_TYPE_DONATION_BOX)
        return;

    TileExtra_DonaitonBox* pTileExtra = pTile->GetExtra<TileExtra_DonaitonBox>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The box is gone!", false);
        return;
    }

    if (auto pButtonClicked = packet.Find("buttonClicked"_hash))
    {
        if (pButtonClicked->valueSize == 0)
            return;

        if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        {
            pPlayer->SendOnTalkBubble("You can't figure out how to open it.", false);
            return;
        }

        std::string_view buttonClicked = pButtonClicked->GetStringView();
        if (buttonClicked == "clear_selected")
        {
            std::vector<bool> selectedGifts;
            const std::string_view accPrefix = "checkbox";

            bool found = false;

            for (uint8 i = 0; i < packet.count; ++i)
            {
                if (packet.fields[i].keySize == 0)
                    continue;

                std::string_view key = packet.fields[i].GetKeyStringView();
                if (key.size() != accPrefix.size() || key.substr(0, accPrefix.size()) != accPrefix)
                    continue;

                found = true;

                bool isChecked;
                if (packet.fields[i].GetBool(isChecked) != TO_INT_SUCCESS)
                {
                    selectedGifts.push_back(false);
                    continue;
                }

                selectedGifts.push_back(isChecked);
            }

            if (!found)
                return;

            EmptyDonationBox(pPlayer, pTile, false, selectedGifts);
        }
        else if (buttonClicked == "clear")
        {
            EmptyDonationBox(pPlayer, pTile, false);
        }
        else
            return;

        if (!pTileExtra->gifts.empty())
        {
            pPlayer->SendOnTalkBubble("`2(Couldn't get all of the gifts)``", false);
        }
        else
        {
            pPlayer->SendOnTalkBubble("`2Box emptied.``", false);
            pPlayer->PlaySFX("page_turn.wav");
        }
    }
    else
    {
        if (auto pItemID = packet.Find("itemid"_hash))
        {
            uint32 itemID = 0;
            if (pItemID->GetUInt(itemID) != TO_INT_SUCCESS)
                return;

            RequestDonatingItem(pPlayer, pTile, itemID);
            return;
        }
    }

    if (auto pMinRarity = packet.Find("min_rarity"_hash))
    {
        int32 minRarity = 0;
        if (pMinRarity->GetInt(minRarity) != TO_INT_SUCCESS)
            return;

        if (minRarity == pTileExtra->minRarity)
            return;

        if (minRarity <= 0)
        {
            pPlayer->SendOnTalkBubble("That not gonna work.", false);
            return;
        }

        if (minRarity > 999)
        {
            pPlayer->SendOnTalkBubble("Min rarity must be from 1-999", false);
            return;
        }

        pTileExtra->minRarity = minRarity;
        pPlayer->SendOnTalkBubble("Updated minimum acceptable item rarity to `2" + ToString(minRarity) + "``!", false);
    }
    else
    {
        pTileExtra->minRarity = 2;
    }
}

void DonationBoxDialog::EmptyDonationBox(GamePlayer* pPlayer, TileInfo* pTile, bool allowDrop,
                                         const std::vector<bool>& selectedGifts)
{
    if (!pPlayer || !pTile)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    TileExtra_DonaitonBox* pTileExtra = pTile->GetExtra<TileExtra_DonaitonBox>();
    if (!pTileExtra)
        return;

    if (pTileExtra->gifts.empty())
    {
        pPlayer->SendOnTalkBubble("`4The box is already empty", false);
        return;
    }

    ItemInfoManager* pItemMgr = GetItemInfoManager();
    UserCacheManager* pUserCacheMgr = GetUserCacheManager();

    int32 index = 0;
    bool isBackpackFull = false;

    auto emptyIfCan = std::remove_if(
        pTileExtra->gifts.begin(), pTileExtra->gifts.end(),
        [&](const TileDonationBoxGifts& gift)
        {
            ItemInfo* pItem = pItemMgr->GetItemByID(gift.itemID);
            if (!pItem)
            {
                index++;
                return true;
            }

            bool takeItem = true;

            if (!selectedGifts.empty())
            {
                if (index >= selectedGifts.size() || !selectedGifts[index])
                {
                    takeItem = false;
                }
            }

            if (takeItem)
            {
                if (pPlayer->GetInventory().GetCountOfItem(pItem->id) + gift.itemCount > pItem->maxCanHold)
                {
                    takeItem = false;
                    isBackpackFull = true;
                }
            }

            index++;

            if (takeItem)
            {
                pPlayer->ModifyInventoryItem(pItem->id, gift.itemCount);

                string consoleMsg;
                consoleMsg.reserve(128);

                auto pUser = pUserCacheMgr->GetMetadata(gift.userID);

                if (pUser)
                {
                    consoleMsg = "[`o" + pPlayer->GetDisplayName(false) + "`w receives `5" + ToString(gift.itemCount) +
                                 "`w " + pItem->name + " from `w" + pUser->displayName + "`o, how nice!]";
                }
                else
                {
                    consoleMsg = "[`o" + pPlayer->GetDisplayName(false) + "`w receives `5" + ToString(gift.itemCount) +
                                 "`w " + pItem->name + "`o, how nice!]";
                }

                pWorld->SendConsoleMessageToAll(consoleMsg);
                return true;
            }
            else if (allowDrop)
            {
                pWorld->DropObjectOnTile(pTile, pItem->id, gift.itemCount, GetRandomItemDropOffset(), true);
                return true;
            }

            return false;
        });

    bool hasChanged = (emptyIfCan != pTileExtra->gifts.end());
    if (hasChanged)
    {
        pTileExtra->gifts.erase(emptyIfCan, pTileExtra->gifts.end());
    }

    if (isBackpackFull)
    {
        pPlayer->SendOnTalkBubble("I dont have enough room in my backpack to get item(s) from the box!", false);
    }

    if (pTileExtra->gifts.empty())
    {
        pTile->RemoveFlag(TILE_FLAG_IS_ON);
        pWorld->SendTileUpdate(pTile);
    }
}

void DonationBoxDialog::RequestDonatingItem(GamePlayer* pPlayer, TileInfo* pTile, int32 itemID)
{
    if (!pPlayer || !pTile)
        return;

    if (pPlayer->GetCurrentWorld() == 0)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
        return;

    InventoryItemInfo* pInvItem = pPlayer->GetInventory().GetItemByID(pItem->id);
    if (!pInvItem)
        return;

    TileExtra_DonaitonBox* pTileExtra = pTile->GetExtra<TileExtra_DonaitonBox>();
    if (!pTileExtra)
        return;

    if (pTileExtra->lastDonationTime.GetElapsedTime() < 5000)
    {
        pPlayer->SendOnTalkBubble("`7[```4The donation box needs to warm up for 5 seconds before use!`7]``", false);
        return;
    }

    if (pItem->rarity != 0 && pItem->rarity < pTileExtra->minRarity && !pItem->HasFlag(ITEM_FLAG_SEEDLESS))
    {
        pPlayer->SendOnTalkBubble("`7[```4This box only accepts items rarity " + ToString(pTileExtra->minRarity) +
                                      "+ or greater`7]``",
                                  false);
        return;
    }

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        ->AddLabelWithIcon(pItem->name, pItem->id, true)
        ->EmbedData("tilex", vTilePos.x)
        ->EmbedData("tiley", vTilePos.y)
        ->EmbedData("itemID", ToItemClientID(pItem->id))
        ->AddTextBox("How many to put in the box as a gift? (Note: You will `4LOSE`` the items you give!)")
        ->AddTextInput("count", "Count:", "", 5)
        ->AddTextInput("sign_text", "Optional Note:", "", 128)
        ->AddSpacer()
        ->AddButton("give", "`4Give the item(s)``")
        ->AddSpacer()
        ->AddButton("cancel", "`wCancel``")
        ->EndDialog("give_item", "", "");

    pPlayer->SendOnDialogRequest(db.Get());
}

void DonationBoxDialog::HandleGiveItem(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pTileX = packet.Find("tilex"_hash);
    if (!pTileX)
        return;

    auto pTileY = packet.Find("tiley"_hash);
    if (!pTileY)
        return;

    auto pButtonClicked = packet.Find("buttonClicked"_hash);
    if (!pButtonClicked || pButtonClicked->valueSize < 4)
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

    ItemInfo* pTileItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pTileItem || pTileItem->type != ITEM_TYPE_DONATION_BOX)
        return;

    TileExtra_DonaitonBox* pTileExtra = pTile->GetExtra<TileExtra_DonaitonBox>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The box is gone!", false);
        return;
    }

    std::string_view buttonClicked = pButtonClicked->GetStringView();
    if (buttonClicked == "cancel")
    {
        Request(pPlayer, pTile, pTileItem);
    }
    else if (buttonClicked == "give")
    {
        auto pItemID = packet.Find("itemID"_hash);
        auto pCount = packet.Find("count"_hash);
        if (!pItemID || !pCount)
            return;

        auto pSignText = packet.Find("sign_text"_hash);
        if (pSignText && pSignText->valueSize > 256)
            return;

        uint32 itemID = 0;
        if (pItemID->GetUInt(itemID) != TO_INT_SUCCESS)
            return;

        ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
        if (!pItem)
            return;

        int32 count = 0;
        if (pCount->GetInt(count) != TO_INT_SUCCESS)
            return;

        if (count == 0)
            return;

        if (count < 0)
        {
            pPlayer->SendOnTalkBubble("That would only have worked if someone programmed this function!", false);
            return;
        }

        if (pTileExtra->gifts.size() >= 20)
        {
            pPlayer->SendOnTalkBubble("You aren't able to fit another gift inside, it's jammed full.", false);
            return;
        }

        string signText = pSignText ? pSignText->GetString() : "";
        RemoveGTColorCodes(signText);
        StripWhiteSpace(signText);

        if (signText.size() > 128)
            return;

        if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        {
            int32 giftCount = pTileExtra->GetCountOfGiftsFromID(pPlayer->GetUserID());
            if (giftCount > 2)
            {
                pPlayer->SendOnTalkBubble("`4Woah nelly, you don't want to be TOO nice.  Try again later.``", false);
                return;
            }
        }

        InventoryItemInfo* pInvItem = pPlayer->GetInventory().GetItemByID(pItem->id);
        if (!pInvItem || pInvItem->count < count || count > pItem->maxCanHold)
        {
            pPlayer->SendOnTalkBubble("You don't have that to give!", false);
            return;
        }

        if (pItem->type == ITEM_TYPE_PETFISH)
        {
            pPlayer->SendOnTalkBubble("You can't put a live fish in the box!  That would just be mean.", false);
            return;
        }

        if (pItem->rarity != 0 && pItem->rarity < pTileExtra->minRarity && !pItem->HasFlag(ITEM_FLAG_SEEDLESS))
        {
            pPlayer->SendOnTalkBubble("`7[```4This box only accepts items rarity " + ToString(pTileExtra->minRarity) +
                                          "+ or greater`7]``",
                                      false);
            return;
        }

        uint32 totalStrLen = signText.size();
        for (auto& gift : pTileExtra->gifts)
        {
            totalStrLen += gift.message.size();
        }

        if (totalStrLen > 2048)
        {
            LOGGER_LOG_ERROR("Failed to write into donate box totalStrLen (with text): %d, text size: %d, userID: %d",
                             totalStrLen, signText.size(), pPlayer->GetUserID());
            return;
        }

        pPlayer->ModifyInventoryItem(pItem->id, -count);
        pTileExtra->gifts.push_back({pPlayer->GetUserID(), signText, (int16)pItem->id, (int16)count});

        pWorld->SendTalkBubbleAndConsoleToAll("`7[`w" + pPlayer->GetDisplayName(false) + "`w places `5" +
                                                  ToString(count) + "`2 " + pItem->name + "`w into the `w" +
                                                  pTileItem->name + "`7]``!",
                                              false, pPlayer);
        pPlayer->PlaySFX("page_turn.wav");

        if (pTileExtra->gifts.size() == 1)
        {
            pTile->SetFlag(TILE_FLAG_IS_ON);
            pWorld->SendTileUpdate(pTile);
        }
    }
}
