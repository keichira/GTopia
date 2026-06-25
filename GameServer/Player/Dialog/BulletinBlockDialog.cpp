#include "BulletinBlockDialog.h"
#include "../GamePlayer.h"
#include "Utils/DialogBuilder.h"
#include "Item/ItemInfoManager.h"
#include "../../World/WorldManager.h"
#include "../../Server/UserCacheManager.h"
#include "../../Player/PlayerManager.h"

void BulletinBlockDialog::Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem)
{
    if(!pPlayer || !pTile || !pItem) 
        return;
    
    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    TileExtra_Bulletin* pTileExtra = pTile->GetExtra<TileExtra_Bulletin>();
    if(!pTileExtra || pItem->type != ITEM_TYPE_BULLETIN) 
        return;

    if(pTileExtra->letters.empty())
    {
        SendBulletinDialog(pPlayer, pWorld, pTile, pItem);
    }
    else
    {
        Vector2Int& vTilePos = pTile->GetPos();
        std::vector<int32> userIDs;
        userIDs.reserve(pTileExtra->letters.size());

        for(auto& letter : pTileExtra->letters)
        {
            userIDs.push_back(letter.userID);
        }

        GetUserCacheManager()->FetchMetadata(
            pPlayer->GetNetID(),
            CACHE_REQ_BULLETIN_BLOCK,
            userIDs,
            { pWorld->GetInstanceID(), vTilePos.x, vTilePos.y }
        );
    }
}

void BulletinBlockDialog::HandleFromCache(GamePlayer* pPlayer, uint32 worldInstanceID, int32 tileX, int32 tileY)
{
    if(!pPlayer || pPlayer->GetCurrentWorld() != worldInstanceID) return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(worldInstanceID);
    if(!pWorld) 
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if(!pTile) 
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if(!pItem || pItem->type != ITEM_TYPE_BULLETIN) 
        return;

    TileExtra_Bulletin* pTileExtra = pTile->GetExtra<TileExtra_Bulletin>();
    if(!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The board is gone!.", false);
        return;
    }

    SendBulletinDialog(pPlayer, pWorld, pTile, pItem);
}

void BulletinBlockDialog::RequestDeleteEntry(GamePlayer* pPlayer, TileInfo* pTile, int32 index)
{
    if(!pTile)
        return;

    TileExtra_Bulletin* pTileExtra = pTile->GetExtra<TileExtra_Bulletin>();
    if(!pTileExtra) 
        return;

    if(index < 0 || pTileExtra->letters.size() <= index)
    {
        pPlayer->SendOnTalkBubble("Can't remove that, it's not there anymore!", false);
        return;
    }

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
    ->AddLabelWithIcon("Delete ``\"" + pTileExtra->letters[index].message + "\"`` from your board?", pTile->GetFG(), false)
    ->EmbedData("tilex", vTilePos.x)
    ->EmbedData("tiley", vTilePos.y)
    ->EmbedData("delete_index", index)
    ->EndDialog("remove_bulletin", "OK", "Cancel");

    pPlayer->SendOnDialogRequest(db.Get());
}

void BulletinBlockDialog::SendBulletinDialog(GamePlayer* pPlayer, World* pWorld, TileInfo* pTile, ItemInfo* pItem)
{
    if(!pPlayer || !pWorld || !pTile || !pItem)
        return;

    if(pPlayer->GetCurrentWorld() != pWorld->GetInstanceID())
        return;

    TileExtra_Bulletin* pTileExtra = pTile->GetExtra<TileExtra_Bulletin>();
    if(!pTileExtra || pItem->type != ITEM_TYPE_BULLETIN) 
        return;

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
      ->AddLabelWithIcon(pItem->name, pItem->id, true)
      ->EmbedData("tilex", vTilePos.x)
      ->EmbedData("tiley", vTilePos.y)
      ->AddSpacer();

    bool hideNames = pTileExtra->HasFlag(TILE_EXTRA_BULLETIN_HIDE_NAMES);
    bool hasAccessToEdit = pWorld->PlayerHasAccessOnTile(pPlayer, pTile);

    if(pTileExtra->letters.empty())
    {
        db.AddTextBox(pItem->name + " is empty.");
    }
    else
    {
        UserCacheManager* pUserMgr = GetUserCacheManager();
        for(uint32 i = 0; i < pTileExtra->letters.size(); ++i)
        {
            UserMetadata* pMetaData = pUserMgr->GetMetadata(pTileExtra->letters[i].userID);
            string shownMsg;
            
            if(!hideNames)
            {
                shownMsg += pMetaData ? pMetaData->displayName + ":`2 " : ("#" + ToString(pTileExtra->letters[i].userID) + ":`2 ");
            }
            shownMsg += pTileExtra->letters[i].message;
            db.AddLabelWithIconButton(ToString(i), shownMsg, ITEM_ID_LETTER);
        }
    }
    db.AddSpacer();

    bool canPostMessage = true;
    if(!hasAccessToEdit && !pTileExtra->HasFlag(TILE_EXTRA_BULLETIN_READ_ONLY))
    {
        uint32 letterCountFromUser = pTileExtra->GetCountOfLettersFromID(pPlayer->GetUserID());
        if(letterCountFromUser > 2)
        {
            db.AddTextBox("You already have `w " + ToString(letterCountFromUser) + "`` posts up, take a break!");
            canPostMessage = false;
        }
    }

    if(hasAccessToEdit)
    {
        db.AddLabelWithIcon("`wOwner Options", ITEM_ID_WORLD_LOCK, true);

        if(hideNames)
            db.AddTextBox("Uncheck `5Hide names`` to enable individual comment removal options.")->AddSpacer();
        else
            db.AddTextBox("To remove an individual comment, press the icon to the left of it.")->AddSpacer();

        if(canPostMessage)
        {
            db.AddTextBox("Add to conversation?")
              ->AddTextInput("sign_text", "", "", 128)
              ->AddSpacer()
              ->AddButton("send", "`2Add");
        }

        if(!pTileExtra->letters.empty())
        {
            db.AddSpacer()->AddButton("clear", "`4Clear Board");
        }

        db.AddCheckBox("checkbox_locked", "Public can add", !pTileExtra->HasFlag(TILE_EXTRA_BULLETIN_READ_ONLY));
        db.AddCheckBox("checkbox_hide", "Hide names", pTileExtra->HasFlag(TILE_EXTRA_BULLETIN_HIDE_NAMES));
    }
    else
    {
        if(canPostMessage && !pTileExtra->HasFlag(TILE_EXTRA_BULLETIN_READ_ONLY))
        {
            db.AddTextBox("Add to conversation?")
              ->AddTextInput("sign_text", "", "", 128)
              ->AddSpacer()
              ->AddButton("send", "`2Add");
        }
    }

    if(hasAccessToEdit)
    {
        db.EndDialog("bulletin_edit", "OK", "Cancel");
    }
    else
    {
        if(!canPostMessage)
            db.EndDialog("bulletin_edit", "", "Cancel");
        else
            db.EndDialog("bulletin_edit", "", "Continue");
    }

    pPlayer->SendOnDialogRequest(db.Get());
}

void BulletinBlockDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
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

    TileExtra_Bulletin* pTileExtra = pTile->GetExtra<TileExtra_Bulletin>();
    if(!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The board is gone!", false);
        return;
    }

    if(pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
    {
        if(auto pLocked = packet.Find("checkbox_locked"_hash))
        {
            bool val;
            if(pLocked->GetBool(val) != TO_INT_SUCCESS)
                return;

            val ? pTileExtra->RemoveFlag(TILE_EXTRA_BULLETIN_READ_ONLY)
                : pTileExtra->SetFlag(TILE_EXTRA_BULLETIN_READ_ONLY);
        }

        if(auto pHide = packet.Find("checkbox_hide"_hash))
        {
            bool val;
            if(pHide->GetBool(val) != TO_INT_SUCCESS)
                return;

            val ? pTileExtra->SetFlag(TILE_EXTRA_BULLETIN_HIDE_NAMES)
                : pTileExtra->RemoveFlag(TILE_EXTRA_BULLETIN_HIDE_NAMES);
        }
    }
    else if(pTileExtra->HasFlag(TILE_EXTRA_BULLETIN_READ_ONLY))
        return;

    if(auto pButtonClicked = packet.Find("buttonClicked"_hash))
    {
        if(pButtonClicked->valueSize == 0 || pButtonClicked->valueSize > 8)
            return;

        std::string_view buttonClicked = pButtonClicked->GetStringView();

        if(buttonClicked == "send")
        {
            auto pSignText = packet.Find("sign_text"_hash);
            if(!pSignText)
                return;

            if(pSignText->valueSize > 128)
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

            if(!pWorld->PlayerHasAccessOnTile(pPlayer, pTile) && pTileExtra->GetCountOfLettersFromID(pPlayer->GetUserID()) > 2)
            {
                pPlayer->SendOnTalkBubble("Don't flood the board.", false);
                return;
            }

            uint32 totalStrLen = text.size();
            for(auto& letter : pTileExtra->letters)
            {
                totalStrLen += letter.message.size();
            }

            if(totalStrLen > 1024)
            {
                LOGGER_LOG_ERROR("Failed to write into bulletin totalStrLen (with text): %d, text size: %d, userID: %d", totalStrLen, text.size(), pPlayer->GetUserID());
                return;
            }

            pTileExtra->letters.push_back({ pPlayer->GetUserID(), text });

            pPlayer->SendOnTalkBubble("`2Bulletin posted.``", false);
            pPlayer->PlaySFX("page_turn.wav");
            return;
        }

        if(buttonClicked == "clear")
        {
            if(!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
            {
                pPlayer->SendOnTalkBubble("It's not yours don't do that.", false);
                return;
            }

            pTileExtra->letters.clear();

            pPlayer->SendOnTalkBubble("`2Text cleared.``", false);
            pPlayer->PlaySFX("page_turn.wav");
            return;
        }

        if(!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
            return;

        int32 index = -1;
        if(pButtonClicked->GetInt(index) != TO_INT_SUCCESS || index < 0)
            return;

        RequestDeleteEntry(pPlayer, pTile, index);
    }
}

void BulletinBlockDialog::HandleDeleteEntry(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if(!pPlayer)
        return;

    auto pTileX = packet.Find("tilex"_hash);
    if(!pTileX)
        return;

    auto pTileY = packet.Find("tiley"_hash);
    if(!pTileY)
        return;

    auto pDeleteIndex = packet.Find("delete_index"_hash);
    if(!pDeleteIndex)
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

    int32 deleteIndex = 0;
    if(pDeleteIndex->GetInt(deleteIndex) != TO_INT_SUCCESS)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if(!pTile)
        return;

    TileExtra_Bulletin* pTileExtra = pTile->GetExtra<TileExtra_Bulletin>();
    if(!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The board is gone!", false);
        return;
    }

    if(deleteIndex < 0 || pTileExtra->letters.size() <= deleteIndex)
    {
        pPlayer->SendOnTalkBubble("Can't remove that, it's not there anymore!", false);
        return;
    }

    if(!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if(!pItem || pItem->type != ITEM_TYPE_BULLETIN)
        return;

    pTileExtra->letters.erase(pTileExtra->letters.begin() + deleteIndex);
    pPlayer->SendOnTalkBubble("`2Bulletin removed.``", false);
    pPlayer->PlaySFX("page_turn.wav");

    if(pItem->id == ITEM_ID_BULLETIN_BOARD)
    {
        BulletinBlockDialog::Request(pPlayer, pTile, pItem);
    }
}
