#include "SignDialog.h"
#include "../GamePlayer.h"
#include "Utils/DialogBuilder.h"
#include "Item/ItemInfoManager.h"
#include "../../World/WorldManager.h"

void SignDialog::Request(GamePlayer* pPlayer, TileInfo* pTile)
{
    if(!pPlayer) {
        return;
    }

    TileExtra_Sign* pTileExtra = pTile->GetExtra<TileExtra_Sign>();
    if(!pTileExtra) {
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetDisplayedItem());
    if(pItem->type != ITEM_TYPE_SIGN) {
        return;
    }

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

void SignDialog::Handle(GamePlayer* pPlayer, const string& text, int32 tileX, int32 tileY)
{
    if(!pPlayer || pPlayer->GetCurrentWorld() == 0) {
        return;
    }

    string written = text;
    RemoveExtraWhiteSpaces(written);

    if(written.size() > 128) {
        pPlayer->SendOnTalkBubble("That text is too long!", false);
        return;
    }

    World* pWorld = GetWorldManager()->GetWorldByID(pPlayer->GetCurrentWorld());
    if(!pWorld) {
        return;
    }

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if(!pTile) {
        pPlayer->SendOnTalkBubble("Huh? The sign is gone!", false);
        printf("pufff");
        return;
    }

    TileExtra_Sign* pTileExtra = pTile->GetExtra<TileExtra_Sign>();
    if(!pTileExtra) {
        pPlayer->SendOnTalkBubble("Huh? The sign is gone!", false);
        printf("pffpffp2");
        return;
    }

    pTileExtra->text = written;
    pWorld->SendTileUpdate(tileX, tileY);
}
