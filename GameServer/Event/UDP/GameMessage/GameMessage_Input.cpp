#include "GameMessage_Input.h"
#include "../../../Server/GameServer.h"
#include "../../../World/WorldManager.h"

#include "../../../Player/Dialog/AchievementBlockDialog.h"
#include "../../../Player/Dialog/BattleCageDialog.h"
#include "../../../Player/Dialog/BulletinBlockDialog.h"
#include "../../../Player/Dialog/DisplayBlockDialog.h"
#include "../../../Player/Dialog/DonationBoxDialog.h"
#include "../../../Player/Dialog/DoorDialog.h"
#include "../../../Player/Dialog/DressupDialog.h"
#include "../../../Player/Dialog/DropItemDialog.h"
#include "../../../Player/Dialog/LockDialog.h"
#include "../../../Player/Dialog/MailboxBlockDialog.h"
#include "../../../Player/Dialog/MannequinDialog.h"
#include "../../../Player/Dialog/OuijaBoardDialog.h"
#include "../../../Player/Dialog/PopupDialog.h"
#include "../../../Player/Dialog/RegisterDialog.h"
#include "../../../Player/Dialog/RenderWorldDialog.h"
#include "../../../Player/Dialog/SignDialog.h"
#include "../../../Player/Dialog/SpotlightDialog.h"
#include "../../../Player/Dialog/SuckerBlockDialog.h"
#include "../../../Player/Dialog/TradeDialog.h"
#include "../../../Player/Dialog/TrashDialog.h"
#include "../../../Player/Dialog/WeatherSpecialDialog.h"
#include "../../../Player/Dialog/XenoniteDialog.h"

DialogReturn::DialogReturn()
{
    RegisterDialogs();
}

void DialogReturn::Execute(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pDialogName = packet.Find("dialog_name"_hash);
    if (!pDialogName || pDialogName->valueSize > 50)
        return;

    std::string_view dialogNameStr = pDialogName->GetStringView();

    if (DialogPagination* pActivePagination = pPlayer->GetActivePaginatedDialog())
    {
        if (pActivePagination->GetDialogName() == dialogNameStr)
        {
            auto pButtonClicked = packet.Find("buttonClicked"_hash);
            if (pButtonClicked)
            {
                if (pActivePagination->ProcessButton(pPlayer, pButtonClicked))
                    return;
            }
        }
        else
            pPlayer->ClosePaginatedDialog();
    }

    uint32 hashedDialogName = HashString(pDialogName->value, pDialogName->valueSize);
    m_dispatcher.Dispatch(hashedDialogName, pPlayer, packet);
}

void DialogReturn::RegisterDialogs()
{
    RegisterDialog<SignDialog::Handle>("sign_edit"_hash);
    RegisterDialog<TrashDialog::Handle>("trash_item"_hash);
    RegisterDialog<TrashDialog::HandleUntradeable>("trash_item2"_hash);
    RegisterDialog<LockDialog::Handle>("lock_edit"_hash);
    RegisterDialog<RenderWorldDialog::Handle>("render_reply"_hash);
    RegisterDialog<RegisterDialog::Handle>("growid_apply"_hash);
    RegisterDialog<DropItemDialog::Handle>("drop_item"_hash);
    RegisterDialog<AchievementBlockDialog::Handle>("achieve_reply"_hash);
    RegisterDialog<OuijaBoardDialog::Handle>("ouijaboard"_hash);
    RegisterDialog<BattleCageDialog::Handle>("battlecage"_hash);
    RegisterDialog<XenoniteDialog::Handle>("xenonite_edit"_hash);
    RegisterDialog<MailboxBlockDialog::Handle>("mailbox_edit"_hash);
    RegisterDialog<PopupDialog::Handle>("plyr_wrench"_hash);
    RegisterDialog<PopupDialog::HandleAcceptAccess>("acceptaccess"_hash);
    RegisterDialog<BulletinBlockDialog::Handle>("bulletin_edit"_hash);
    RegisterDialog<BulletinBlockDialog::HandleDeleteEntry>("remove_bulletin"_hash);
    RegisterDialog<PopupDialog::HandleTitleEdit>("title_edit"_hash);
    RegisterDialog<DoorDialog::Handle>("door_edit"_hash);
    RegisterDialog<DoorDialog::HandlePasswordReply>("password_reply"_hash);
    RegisterDialog<TradeDialog::Handle>("trade_item"_hash);
    RegisterDialog<DonationBoxDialog::Handle>("donation_box_edit"_hash);
    RegisterDialog<DonationBoxDialog::HandleGiveItem>("give_item"_hash);
    RegisterDialog<PopupDialog::HandleBillboardEdit>("billboard_edit"_hash);
    RegisterDialog<WeatherSpecialDialog::Handle>("weatherspcl"_hash);
    RegisterDialog<MannequinDialog::Handle>("mannequin_edit"_hash);
    RegisterDialog<DressupDialog::Handle>("dressup_edit"_hash);
    RegisterDialog<DressupDialog::HandleAsk>("dressup_ask"_hash);
    RegisterDialog<SpotlightDialog::Handle>("spotlight"_hash);
    RegisterDialog<DisplayBlockDialog::Handle>("displayblock"_hash);
    RegisterDialog<SuckerBlockDialog::Handle>("itemsucker"_hash);
    RegisterDialog<SuckerBlockDialog::HandleAddItem>("itemaddedtosucker"_hash);
    RegisterDialog<SuckerBlockDialog::HandleRetrieveItem>("itemremovedfromsucker"_hash);
}

void GameMessage_DialogReturn(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    dialogReturnMgr.Execute(pPlayer, packet);
}

void GameMessage_SetSkin(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    if (auto pColor = packet.Find("color"_hash))
    {
        uint32 skinColor = 0;
        if (pColor->GetUInt(skinColor) != TO_INT_SUCCESS)
            return;

        pPlayer->GetCharData().SetSkinColor(pPlayer->NormalizeSkinColor(skinColor));
    }
}

void GameMessage_Input(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    auto pText = packet.Find("text"_hash);
    if (!pText)
        return;

    string text(pText->value, pText->valueSize);

    RemoveExtraWhiteSpaces(text);
    if (text.empty())
        return;

    if (text[0] == '/')
    {
        pPlayer->SendOnConsoleMessage("`o" + text);

        auto args = Split(text, ' ');
        GetGameServer()->ExecuteCommand(pPlayer, args);
        return;
    }
}

void GameMessage_Wrench(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    if (auto pNetID = packet.Find("netid"_hash))
    {
        uint32 netID = 0;
        if (pNetID->GetUInt(netID) != TO_INT_SUCCESS)
            return;

        if (pPlayer->GetNetID() == netID)
            PopupDialog::RequestSelf(pPlayer);
    }
}
