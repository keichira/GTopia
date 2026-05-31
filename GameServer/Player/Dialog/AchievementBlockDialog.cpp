#include "AchievementBlockDialog.h"
#include "Item/ItemInfoManager.h"
#include "../../World/WorldManager.h"
#include "../GamePlayer.h"
#include "Utils/DialogBuilder.h"

void AchievementBlockDialog::Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem)
{
    if(!pPlayer || !pTile || !pItem)
        return;

    if(pItem->type != ITEM_TYPE_ACHIEVEMENT)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    if(pWorld->GetWorldOwnerID() != pPlayer->GetUserID())
    {
        pPlayer->SendOnTalkBubble("An `5Achievement Block`` can only be etched by the owner of this world. (Requires `5World Lock``)", false);
        return;
    }

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.AddLabelWithIcon("`w" + pItem->name + "``", pItem->id, true)
    ->EmbedData("tilex", vTilePos.x)
    ->EmbedData("tiley", vTilePos.y)
    ->AddTextBox("Which design do you want to etch into the block? (Tap the icon)");

    if(pPlayer->GetProgressData().BuildAchievementsDialog(db, true) == 0)
    {
        db.AddTextBox("(`4Oops, you haven't earned any achievements yet, come back later!``)");
    }

    db.EndDialog("achieve_reply", "Clear It", "Cancel");
    pPlayer->SendOnDialogRequest(db.Get());
}

void AchievementBlockDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    if(!pPlayer)
        return;

    auto pTileX = packet.Find("tilex"_hash);
    if(!pTileX)
        return;

    auto pTileY = packet.Find("tiley"_hash);
    if(!pTileY)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    int32 tileX = 0;
    if(ToInt(string(pTileX->value, pTileX->size), tileX) != TO_INT_SUCCESS)
        return;

    int32 tileY = 0;
    if(ToInt(string(pTileY->value, pTileY->size), tileY) != TO_INT_SUCCESS)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if(!pTile)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetDisplayedItem());
    if(!pItem || pItem->type != ITEM_TYPE_ACHIEVEMENT)
        return;

    if(pPlayer->GetUserID() != pWorld->GetWorldOwnerID())
    {
        pPlayer->SendOnTalkBubble("Only the owner of the `$World Lock`` can etch these blocks.", true);
        return;
    }

    TileExtra_Achievement* pTileExtra = pTile->GetExtra<TileExtra_Achievement>();
    if(!pTileExtra)
        return;

    int32 achievementID = 127;
    auto pButtonClicked = packet.Find("buttonClicked"_hash);
    if(pButtonClicked)
    {
        if(ToInt(string(pButtonClicked->value, pButtonClicked->size), achievementID) != TO_INT_SUCCESS)
            return;
    }

    if(achievementID != 127 && achievementID > ACHIEVEMENT_COUNT && !pPlayer->GetProgressData().HasAchievement((eAchievement)achievementID))
        return;

    string message = "Block etched.";
    if(achievementID == 127)
    {
        message = "`5Achievement Block`` cleared.";
    }

    pTileExtra->achievementID = achievementID;
    pWorld->SendTileUpdate(pTile);
    pPlayer->SendOnTalkBubble(message, false);
}
