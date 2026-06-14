#include "MailboxBlockDialog.h"
#include "../GamePlayer.h"
#include "Utils/DialogBuilder.h"
#include "Item/ItemInfoManager.h"
#include "../../World/WorldManager.h"
#include "../../Server/UserCacheManager.h"
#include "../../Player/PlayerManager.h"

void MailboxBlockDialog::Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem)
{
    if(!pPlayer || !pTile || !pItem)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    TileExtra_Mailbox* pTileExtra = pTile->GetExtra<TileExtra_Mailbox>();
    if(!pTileExtra)
        return;

    if(pItem->type != ITEM_TYPE_MAILBOX)
        return;

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
    ->AddLabelWithIcon(pItem->name, pItem->id, true)
    ->EmbedData("tilex", vTilePos.x)
    ->EmbedData("tiley", vTilePos.y);

    if(pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
    {
        if(pTileExtra->letters.empty())
        {
            db.AddTextBox("Your mailbox is currently empty.")
            ->AddTextBox("Write a letter to yourself?")
            ->AddTextInput("sign_text", "", "", 128)
            ->AddSpacer()
            ->AddButton("send", "`2Send Letter``");
        }
        else
        {
            std::vector<int32> userIDs;
            userIDs.reserve(pTileExtra->letters.size());

            for(auto& letter : pTileExtra->letters) 
            {
                userIDs.push_back(letter.userID);
            }

            GetUserCacheManager()->FetchMetadata(
                pPlayer->GetNetID(),
                CACHE_REQ_MAILBOX_BLOCK,
                userIDs,
                { pWorld->GetInstanceID(), vTilePos.x, vTilePos.y }
            );
        }
    }
    else
    {
        bool canWrite = true;

        if(pTileExtra->letters.size() >= 20)
        {
            db.AddTextBox("This mailbox already has `w" + ToString(pTileExtra->letters.size()) + "`` letters in it. Try again later.");
            canWrite = false;
        }

        if(pTileExtra->HasLetterFromID(pPlayer->GetUserID()))
        {
            if(canWrite)
            {
                db.AddTextBox("You've already crammed `w1`` of your letters into the mailbox, better wait.");
            }
        }
        else if(canWrite)
        {
            db.AddTextBox("Want to leave a message for the owner?")
            ->AddTextInput("sign_text", "", "", 128)
            ->AddSpacer()
            ->AddButton("send", "`2Send Letter``");
        }
    }

    db.EndDialog("mailbox_edit", "", "Cancel");
    pPlayer->SendOnDialogRequest(db.Get());
}

void MailboxBlockDialog::HandleFromCache(GamePlayer* pPlayer, uint32 worldInstanceID, int32 tileX, int32 tileY)
{
    if(!pPlayer || pPlayer->GetCurrentWorld() != worldInstanceID)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(worldInstanceID);
    if(!pWorld)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if(!pTile)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if(pItem->type != ITEM_TYPE_MAILBOX)
        return;

    TileExtra_Mailbox* pTileExtra = pTile->GetExtra<TileExtra_Mailbox>();
    if(!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The mailbox is gone!.", false);
        return;
    }

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
    ->AddLabelWithIcon(pItem->name, pItem->id, true)
    ->EmbedData("tilex", vTilePos.x)
    ->EmbedData("tiley", vTilePos.y)
    ->AddTextBox("You have `w" + ToString(pTileExtra->letters.size()) + "`` letter" + (pTileExtra->letters.size() > 1 ? "s:" : ":"))
    ->AddSpacer();

    UserCacheManager* pUserMgr = GetUserCacheManager();

    for(auto& letter : pTileExtra->letters)
    {
        UserMetadata* pMetaData = pUserMgr->GetMetadata(letter.userID);

        db.AddLabelWithIcon("`#\"" + letter.message + "\" - `w" + (pMetaData ? pMetaData->displayName : ToString(letter.userID)), ITEM_ID_LETTER)
        ->AddSpacer();
    }

    if(pTileExtra->letters.size() >= 20)
    {
        db.AddTextBox("This mailbox already has `w" + ToString(pTileExtra->letters.size()) + "`` letters in it, can't add more until you clear them.");
    }
    else
    {
        db.AddTextBox("Write a letter to yourself?")
        ->AddTextInput("sign_text", "", "", 128)
        ->AddSpacer()
        ->AddButton("send", "`2Send Letter``");
    }

    db.AddSpacer()
    ->AddButton("clear", "`4Empty Mailbox``")
    ->EndDialog("mailbox_edit", "", "Cancel");

    pPlayer->SendOnDialogRequest(db.Get());
}

void MailboxBlockDialog::Handle(GamePlayer *pPlayer, ParsedTextPacket<40> &packet)
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

    auto pButtonClicked = packet.Find("buttonClicked"_hash);
    if(!pButtonClicked)
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

    TileExtra_Mailbox* pTileExtra = pTile->GetExtra<TileExtra_Mailbox>();
    if(!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The mailbox is gone!", false);
        return;
    }

    std::string_view buttonClicked = pButtonClicked->GetStringView();

    if(buttonClicked == "send")
    {
        auto pSignText = packet.Find("sign_text"_hash);
        if(!pSignText)
            return;

        if(pSignText->size > 128)
        {
            pPlayer->SendOnTalkBubble("That letter is too long!", false);
            return;
        }
    
        string text = pSignText->GetString();
        RemoveExtraWhiteSpaces(text);
        RemoveGTColorCodes(text);

        if(text.empty() || text.size() > 128)
            return;

        if(text.size() < 3)
        {
            pPlayer->SendOnTalkBubble("That's not interesting enough to mail.", false);
            return;
        }

        if(pTileExtra->letters.size() >= 20)
        {
            pPlayer->SendOnTalkBubble("You aren't able to fit another letter inside, it's jammed full.", false);
            return;
        }

        if(!pWorld->PlayerHasAccessOnTile(pPlayer, pTile) && pTileExtra->HasLetterFromID(pPlayer->GetUserID()))
        {
            pPlayer->SendOnTalkBubble("Don't flood the mailbox.", false);
            return;
        }

        uint32 totalStrLen = text.size();
        for(auto& letter : pTileExtra->letters)
        {
            totalStrLen += letter.message.size();
        }

        if(totalStrLen > 1024)
        {
            LOGGER_LOG_ERROR("Failed to write into mailbox totalStrLen (with text): %d, text size: %d, userID: %d", totalStrLen, text.size(), pPlayer->GetUserID());
            return;
        }

        pTileExtra->letters.push_back({ pPlayer->GetUserID(), text });

        pPlayer->SendOnTalkBubble("`2You placed your letter in the mailbox.``", false);
        pPlayer->PlaySFX("page_turn.wav");

        if(pTileExtra->letters.size() == 1)
        {
            pTile->SetFlag(TILE_FLAG_IS_ON);
            pWorld->SendTileUpdate(pTile);
        }

        return;
    }

    if(buttonClicked == "clear")
    {
        if(!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        {
            pPlayer->SendOnTalkBubble("You can't figure out how to open it.", false);
            return;
        }

        if(pTileExtra->letters.size() > 0)
        {
            pTile->RemoveFlag(TILE_FLAG_IS_ON);
            pWorld->SendTileUpdate(pTile);
        }

        pTileExtra->letters.clear();

        pPlayer->SendOnTalkBubble("`2Mailbox emptied.``", false);
        pPlayer->PlaySFX("page_turn.wav");
        return;
    }
}
