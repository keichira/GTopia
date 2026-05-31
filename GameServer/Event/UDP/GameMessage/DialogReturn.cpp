#include "DialogReturn.h"
#include "IO/Log.h"

#include "../../../Player/Dialog/SignDialog.h"
#include "../../../Player/Dialog/TrashDialog.h"
#include "../../../Player/Dialog/LockDialog.h"
#include "../../../Player/Dialog/RenderWorldDialog.h"
#include "../../../Player/Dialog/RegisterDialog.h"
#include "../../../Player/Dialog/DropItemDialog.h"
#include "../../../Player/Dialog/AchievementBlockDialog.h"
#include "../../../Player/Dialog/OuijaBoardDialog.h"
#include "../../../Player/Dialog/BattleCageDialog.h"

void DialogReturn::Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    if(!pPlayer)
        return;

    auto pDialogName = packet.Find("dialog_name"_hash);
    if(!pDialogName || pDialogName->size > 50)
        return;

    uint32 hashedDialogName = HashString(pDialogName->value, pDialogName->size);

    switch(hashedDialogName) 
    {
        case "sign_edit"_hash: 
        {
            auto pTileX = packet.Find("tilex"_hash);
            auto pTileY = packet.Find("tiley"_hash);
            auto pSignText = packet.Find("sign_text"_hash);

            if(!pTileX || !pTileY || !pSignText)
                return;

            // we need int converter that supports non null term
            // idk if its good ways to convert it to a str
            // really we need it??

            int32 tileX = 0;
            if(ToInt(string(pTileX->value, pTileX->size), tileX) != TO_INT_SUCCESS)
                return;

            int32 tileY = 0;
            if(ToInt(string(pTileY->value, pTileY->size), tileY) != TO_INT_SUCCESS)
                return;

            SignDialog::Handle(pPlayer, string(pSignText->value, pSignText->size), tileX, tileY);
            break;
        }

        case "trash_item"_hash:
        case "trash_item2"_hash: 
        {
            auto pItemID = packet.Find("itemID"_hash);
            auto pCount = packet.Find("count"_hash);

            if(!pItemID || !pCount)
                return;

            uint32 itemID = 0;
            if(ToUInt(string(pItemID->value, pItemID->size), itemID) != TO_INT_SUCCESS)
                return;

            int32 count = 0;
            if(ToInt(string(pCount->value, pCount->size), count) != TO_INT_SUCCESS)
                return;

            if(hashedDialogName == "trash_item2"_hash) 
            {
                TrashDialog::HandleUntradeable(pPlayer, itemID, count);
            }
            else 
            {
                TrashDialog::Handle(pPlayer, itemID, count);
            }

            break;
        }

        case "lock_edit"_hash: 
        {
            LockDialog::Handle(pPlayer, packet);
            break;
        }

        case "render_reply"_hash: 
        {
            RenderWorldDialog::Handle(pPlayer);
            break;
        }

        case "growid_apply"_hash: 
        {
            RegisterDialog::Handle(pPlayer, packet);
            break;
        }

        case "drop_item"_hash:
        {
            DropItemDialog::Handle(pPlayer, packet);
            break;
        }

        case "achieve_reply"_hash:
        {
            AchievementBlockDialog::Handle(pPlayer, packet);
            break;
        }

        case "ouijaboard"_hash:
        {
            OuijaBoardDialog::Handle(pPlayer, packet);
            break;
        }

        case "battlecage"_hash:
        {
            BattleCageDialog::Handle(pPlayer, packet);
            break;
        }
    }
}