#include "DialogManager.h"
#include "../Player/GamePlayer.h"
#include "GameDialogs.h"

void DialogManager::Execute(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
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

void DialogManager::RegisterAllDialogs()
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
    RegisterDialog<GatewayDialog::Handle>("gateway_edit"_hash);
}

DialogManager* GetDialogManager()
{
    return DialogManager::GetInstance();
}
