#include "RenderWorldDialog.h"
#include "../../Context.h"
#include "../../Player/PlayerManager.h"
#include "../../Server/MasterBroadway.h"
#include "../../World/WorldManager.h"
#include "Utils/DialogBuilder.h"

void RenderWorldDialog::Request(GamePlayer* pPlayer)
{
    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon("Render World", ITEM_ID_MAILBOX, true)
        .AddTextBox(
            "World rendering means we'll make a picture of your `5ENTIRE world`` and host it on our server publicly, "
            "for anybody to view.<CR><CR>`4Warning:`` This picture will also include you and all co-owners on your "
            "`5World Lock``.<CR>If you'd like to keep that information private, click Cancel!")
        .EndDialog("render_reply", "Render it!", "Cancel");

    pPlayer->SendOnDialogRequest(db.Get());
}

void RenderWorldDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (pPlayer->GetCurrentWorld() == 0)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    pWorld->SaveToDatabase();
    pPlayer->SetState(PLAYER_STATE_RENDERING_WORLD);

    // temp, it very bad practice but for now keep it (lazy ;-;)
    QueryRequest req;
    req.callback = [](QueryTaskResult&& res)
    {
        if (res.extraData.size() < 1)
            return;

        GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(res.extraData[0].GetUINT());
        if (!pPlayer)
            return;

        pPlayer->SendOnConsoleMessage("Rendering world into a picture.  It might take a few seconds, keep playing and "
                                      "we'll let you know when it's ready.");
        GetMasterBroadway()->SendRenderWorldRequest(pPlayer->GetUserID(), pPlayer->GetCurrentWorld());
    };
    req.AddExtraData(pPlayer->GetNetID());
    DatabaseExec(GetContext()->GetDatabasePool(), "", req, QUERY_FLAG_NONE);
}

void RenderWorldDialog::OnRendered(GamePlayer* pPlayer, const string& worldName)
{
    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon("World Render Share", ITEM_ID_MAILBOX, true)
        .AddTextBox("Your world `#" + worldName + " `ohas been rendered!<CR>You can view it on our discord server.")
        .EndDialog("render_share", "", "Thanks!");

    pPlayer->SendOnDialogRequest(db.Get());
}
