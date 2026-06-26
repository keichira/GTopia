#include "SignDialog.h"
#include "../GamePlayer.h"
#include "Utils/DialogBuilder.h"
#include "Item/ItemInfoManager.h"
#include "../../World/WorldManager.h"

void SignDialog::Request(GamePlayer* pPlayer, TileInfo* pTile)
{
    if(!pPlayer)
        return;

    TileExtra_Sign* pTileExtra = pTile->GetExtra<TileExtra_Sign>();
    if(!pTileExtra)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if(!pItem)
        return;

    DialogBuilder db;
    db.SetDefaultColor('o')
    ->AddLabelWithIcon("`wEdit " + pItem->name, pItem->id, true);

    if(pItem->type == ITEM_TYPE_CHAL_FLAG)
    {
        db.AddTextBox("Enter an ID. This flag will be connected to the Challenge Timer with the same ID.``");
    }
    else if(IsPathMarker(pItem->id))
    {
        db.AddTextBox("Enter an ID. You can use this as a destination for Doors.``");
    }
    else
    {
        db.AddTextBox("What would you like to write on this sign?``");
    }
   
    db.AddTextInput("sign_text", "", pTileExtra->text, 128)
    ->EmbedData("tilex", pTile->GetPos().x)
    ->EmbedData("tiley", pTile->GetPos().y)
    ->EndDialog("sign_edit", "OK", "Cancel");

    pPlayer->SendOnDialogRequest(db.Get());
}

void SignDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if(!pPlayer)
        return;

    auto pTileX = packet.Find("tilex"_hash);
    if(!pTileX)
        return;

    auto pTileY = packet.Find("tiley"_hash);
    if(!pTileY)
        return;

    auto pSignText = packet.Find("sign_text"_hash);
    if(!pSignText)
        return;

    if(pSignText->valueSize > 128)
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

    TileExtra_Sign* pTileExtra = pTile->GetExtra<TileExtra_Sign>();
    if(!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The sign is gone!", false);
        return;
    }

    if(!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if(!pItem)
        return;

    if(pItem->type == ITEM_TYPE_RACE_FLAG)
    {
        if(pSignText->valueSize > 11)
        {
            pPlayer->SendOnTalkBubble("That ID is too long!", false);
            return;
        }

        string raceID = ToUpper(pSignText->GetString());
        RemoveGTColorCodes(raceID);
        pTileExtra->text = raceID;
    }
    else if(IsPathMarker(pItem->id))
    {
        if(pSignText->valueSize > 11)
        {
            pPlayer->SendOnTalkBubble("That door ID is too long!", false);
            return;
        }

        string doorID = ToUpper(pSignText->GetString());
        RemoveGTColorCodes(doorID);
        pTileExtra->text = doorID;
    }
    else
    {
        if(pSignText->GetStringView().find("__&%@PL@%&__") != string::npos)
        {
            pPlayer->SendOnTalkBubble("You try to write the magic symbols down but they disappear!", false);
            return;
        }

        string text = pSignText->GetString();
        RemoveGTColorCodes(text);

        pTileExtra->text = text;
        pWorld->SendTileUpdate(tileX, tileY);
    }
}
