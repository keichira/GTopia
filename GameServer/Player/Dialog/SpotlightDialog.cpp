#include "SpotlightDialog.h"
#include "../../World/WorldManager.h"
#include "../GamePlayer.h"
#include "../PlayerManager.h"
#include "Item/ItemInfoManager.h"
#include "Utils/DialogBuilder.h"

void SpotlightDialog::Request(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile)
        return;

    TileExtra_Spotlight* pTileExtra = pTile->GetExtra<TileExtra_Spotlight>();
    if (!pTileExtra)
        return;

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon("`wShine the Spotlight!``", pTile->GetFG(), true)
        .AddSpacer()
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y);

    GamePlayer* pTarget = GetPlayerManager()->GetPlayerByNetID(pTileExtra->playerNetID);
    if (!pTarget || pTarget->GetCurrentWorld() != pPlayer->GetCurrentWorld())
    {
        db.AddTextBox("The light is currently off.")
            .AddSpacer()
            .AddPlayerPicker("playerNetID", "`wChosee a superstar``");
    }
    else
    {
        db.AddTextBox("The light is shining on " + pTarget->GetRawName())
            .AddSpacer()
            .AddPlayerPicker("playerNetID", "`wChoose a new star``")
            .AddButton("off", "Turn it off");
    }

    db.EndDialog("spotlight", "", "Nevermind");
    pPlayer->SendOnDialogRequest(db.Get());
}

void SpotlightDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pTileX = packet.Find("tilex"_hash);
    if (!pTileX)
        return;

    auto pTileY = packet.Find("tiley"_hash);
    if (!pTileY)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    int32 tileX = 0;
    if (pTileX->GetInt(tileX) != TO_INT_SUCCESS)
        return;

    int32 tileY = 0;
    if (pTileY->GetInt(tileY) != TO_INT_SUCCESS)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if (!pTile)
        return;

    TileExtra_Spotlight* pTileExtra = pTile->GetExtra<TileExtra_Spotlight>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The spotlight is gone!", false);
        return;
    }

    if (auto pButtonClicked = packet.Find("buttonClicked"_hash))
    {
        if (pButtonClicked->GetStringView() != "off")
            return;

        GamePlayer* pTarget = GetPlayerManager()->GetPlayerByNetID(pTileExtra->playerNetID);
        if (pTarget && pTarget->GetCurrentWorld() == pPlayer->GetCurrentWorld())
        {
            pTarget->GetModController().RemovePlayMod(PLAYMOD_TYPE_IN_THE_SPOTLIGHT);
            pTileExtra->playerNetID = 0;
            pTarget->SendOnTalkBubble("Lights out!", false);
        }
    }

    if (auto pPlayerNetID = packet.Find("playerNetID"_hash))
    {
        uint32 netID = 0;
        if (pPlayerNetID->GetUInt(netID) != TO_INT_SUCCESS)
            return;

        GamePlayer* pTarget = GetPlayerManager()->GetPlayerByNetID(netID);
        if (pTarget && pTarget->GetCurrentWorld() == pPlayer->GetCurrentWorld())
        {
            GamePlayer* pOldTarget = GetPlayerManager()->GetPlayerByNetID(pTileExtra->playerNetID);
            if (pOldTarget && pOldTarget->GetCurrentWorld() == pPlayer->GetCurrentWorld())
            {
                pOldTarget->GetModController().RemovePlayMod(PLAYMOD_TYPE_IN_THE_SPOTLIGHT);
            }

            pTileExtra->playerNetID = pTarget->GetNetID();
            pTarget->GetModController().AddPlayMod(PLAYMOD_TYPE_IN_THE_SPOTLIGHT);

            TileInfo* pNextSpot = pWorld->GetTileManager()->GetTileInfoByItemID(pTile->GetFG());
            while (pNextSpot)
            {
                if (pNextSpot != pTile)
                {
                    TileExtra_Spotlight* pNextExtra = pNextSpot->GetExtra<TileExtra_Spotlight>();
                    if (!pNextExtra)
                        continue;

                    if (pNextExtra->playerNetID == pTarget->GetNetID())
                        pTileExtra->playerNetID = 0;
                }

                pNextSpot = pWorld->GetTileManager()->GetTileInfoByItemID(pTile->GetFG(), pNextSpot->GetMapIndex() + 1);
            }

            string notifMsg = "You shine the light on ";
            if (pTarget == pPlayer)
                notifMsg += "yourself";
            else
                notifMsg += pTarget->GetRawName();
            notifMsg += "!";

            pPlayer->SendOnTalkBubble(notifMsg, false);
        }
    }
}
