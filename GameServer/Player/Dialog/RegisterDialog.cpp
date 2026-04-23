#include "RegisterDialog.h"
#include "../GamePlayer.h"
#include "Utils/DialogBuilder.h"
#include "../../World/WorldManager.h"

void RegisterDialog::Request(GamePlayer* pPlayer, const string& namePlaceholder, const string& passPlaceholder, const string& passVerifPlaceholder, const string& errorMsg)
{
    if(!pPlayer || pPlayer->HasGrowID()) {
        return;
    }

    DialogBuilder db;
    db.SetDefaultColor('o')
    ->AddLabelWithIcon("`wGet a GrowID", ITEM_ID_FIST, true)
    ->AddSpacer();
    
    if(!errorMsg.empty()) {
        db.AddTextBox(errorMsg)
        ->AddSpacer();
    }
    
    db.AddTextBox("By choosing a `wGrowID``, you can use a name and password to logon from any device. Your `wname`` will be shown to other players!")
    ->AddTextInput("logon", "Name", namePlaceholder, 18)
    ->AddSpacer()
    ->AddTextInputPassword("password", "Password", passPlaceholder, 18)
    ->AddTextInputPassword("verify_password", "Verify Password", passVerifPlaceholder, 18)
    ->AddSpacer()
    ->EndDialog("growid_apply", "Get My GrowID!", "Cancel");

    pPlayer->SendOnDialogRequest(db.Get());
}

void RegisterDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    if(!pPlayer || pPlayer->HasGrowID()) {
        return;
    }

    auto pName = packet.Find(CompileTimeHashString("logon"));
    auto pPass = packet.Find(CompileTimeHashString("password"));
    auto pVerifPass = packet.Find(CompileTimeHashString("verify_password"));

    if(!pName || !pPass || !pVerifPass) {
        return;
    }

    string name = string(pName->value, pName->size);
    string pass = string(pPass->value, pPass->size);
    string verifPass = string(pVerifPass->value, pVerifPass->size);
    string error = "";

    if(name.find(" ") != string::npos) error = "`4Oops!``  Your `$GrowID`` name can't have spaces in it.";
    else if(name.find("`") != string::npos) error = "`4Oops!`` You can't use color codes in your `$GrowID``.";
    else if(pass != verifPass) error = "`4Oops!``  Passwords don't match.  Try again.";
    else if(pass.size() < 3 || pass.size() > 18) error = "`4Oops!``  Your password must be between `$3`` and `$12`` characters long.";
    else if(name.size() < 3 || name.size() > 12) error = "`4Oops!``  Your `wGrowID`` must be between `$3`` and `$12`` characters long.";
    
    if(!error.empty()) {
        Request(
            pPlayer, name,
            pass, verifPass, error
        );
        return;
    }

    VariantVector extraData(3);
    extraData[0] = name;
    extraData[1] = pass;
    extraData[2] = verifPass;

    pPlayer->CheckLimitsForAccountCreation(true, extraData);
}

void RegisterDialog::Success(GamePlayer* pPlayer, const string& growID, const string& pass)
{
    DialogBuilder db;
    db.SetDefaultColor('o')
    ->AddLabelWithIcon("`wGrowID GET!", ITEM_ID_FIST, true)
    ->AddTextBox("A `wGrowID`` with the log on of `w" + growID + "`` and the password of `w" + pass + "`` created. Write them down, they will be required to log on from now on!")
    ->EndDialog("growid_succ", "", "Continue");

    pPlayer->SendSetHasGrowID(true);

    pPlayer->SendOnDialogRequest(db.Get());
    pPlayer->PlaySFX("piano_nice.wav");

    if(pPlayer->GetCurrentWorld() != 0) {
        World* pWorld = GetWorldManager()->GetWorldByID(pPlayer->GetCurrentWorld());
        if(pWorld) {
            pWorld->SendNameChangeToAll(pPlayer);
        }
    }
}
