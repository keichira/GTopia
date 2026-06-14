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

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetDisplayedItem());
    if(pItem->type != ITEM_TYPE_SIGN)
        return;

    DialogBuilder db;
    db.SetDefaultColor('o')
    ->AddLabelWithIcon("`wEdit " + pItem->name, pItem->id, true)
    ->AddSpacer()
    ->AddTextBox("What would you like to write on this sign?")
    ->AddTextInput("sign_text", "", pTileExtra->text, 128)
    ->EmbedData("tilex", pTile->GetPos().x)
    ->EmbedData("tiley", pTile->GetPos().y)
    ->EndDialog("sign_edit", "OK", "Cancel");

    pPlayer->SendOnDialogRequest(db.Get());
}

void SignDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<40>& packet)
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

    if(pSignText->size == 0)
        return;

    if(pSignText->size > 128)
    {
        pPlayer->SendOnTalkBubble("That text is too long!", false);
        return;
    }

    string text = pSignText->GetString();
    RemoveExtraWhiteSpaces(text);

    pTileExtra->text = text;
    pWorld->SendTileUpdate(tileX, tileY);
}
