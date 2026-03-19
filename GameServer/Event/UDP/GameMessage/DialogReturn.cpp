#include "DialogReturn.h"
#include "IO/Log.h"

#include "../../../Player/Dialog/SignDialog.h"
#include "../../../Player/Dialog/TrashDialog.h"

void DialogReturn::Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    if(!pPlayer) {
        return;
    }

    auto pDialogName = packet.Find(CompileTimeHashString("dialog_name"));
    if(!pDialogName || pDialogName->size > 50) {
        return;
    }

    uint32 hashedDialogName = HashString(pDialogName->value, pDialogName->size);

    switch(hashedDialogName) {
        case CompileTimeHashString("sign_edit"): {
            auto pTileX = packet.Find(CompileTimeHashString("tilex"));
            auto pTileY = packet.Find(CompileTimeHashString("tiley"));
            auto pSignText = packet.Find(CompileTimeHashString("sign_text"));

            if(!pTileX || !pTileY || !pSignText) {
                return;
            }

            // we need to int converter that supports non null term
            // idk if its good ways to convert it to a str
            // really we need it??

            int32 tileX = 0;
            if(ToInt(string(pTileX->value, pTileX->size), tileX) != TO_INT_SUCCESS) {
                return;
            }

            int32 tileY = 0;
            if(ToInt(string(pTileY->value, pTileY->size), tileY) != TO_INT_SUCCESS) {
                return;
            }

            SignDialog::Handle(pPlayer, string(pSignText->value, pSignText->size), tileX, tileY);
            break;
        }

        case CompileTimeHashString("trash_item"):
        case CompileTimeHashString("trash_item2"): {
            auto pItemID = packet.Find(CompileTimeHashString("itemID"));
            auto pCount = packet.Find(CompileTimeHashString("count"));

            if(!pItemID || !pCount) {
                return;
            }

            uint32 itemID = 0;
            if(ToUInt(string(pItemID->value, pItemID->size), itemID) != TO_INT_SUCCESS) {
                return;
            }

            int32 count = 0;
            if(ToInt(string(pCount->value, pCount->size), count) != TO_INT_SUCCESS) {
                return;
            }

            if(hashedDialogName == CompileTimeHashString("trash_item2")) {
                TrashDialog::HandleUntradeable(pPlayer, itemID, count);
            }
            else {
                TrashDialog::Handle(pPlayer, itemID, count);
            }
            break;
        }
    }
}