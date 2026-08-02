#include "DressupDialog.h"
#include "../../World/WorldManager.h"
#include "../GamePlayer.h"
#include "Item/ItemInfoManager.h"
#include "Utils/DialogBuilder.h"

void DressupDialog::Request(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile)
        return;

    if (!pPlayer || !pTile)
        return;

    TileExtra_Dressup* pTileExtra = pTile->GetExtra<TileExtra_Dressup>();
    if (!pTileExtra)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem)
        return;

    if (pItem->type != ITEM_TYPE_DRESSUP)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon("`w" + pItem->name + "``", pItem->id, true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y);

    if (!pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
    {
        db.AddTextBox("This is somebody else's " + pItem->name + ".").EndDialog("dressup_edit", "", "Exit");
        pPlayer->SendOnDialogRequest(db.Get());
        return;
    }

    bool hasAnyClothing = false;
    for (int32 i = 0; i < 9; ++i)
    {
        if (pTileExtra->clothes[i] == ITEM_ID_BLANK)
            continue;

        if (!hasAnyClothing)
            db.AddTextBox("Your " + pItem->name + " contains a neatly folded outfit:");

        hasAnyClothing = true;

        ItemInfo* pCloth = GetItemInfoManager()->GetItemByID(pTileExtra->clothes[i]);
        if (!pCloth)
            continue;

        db.AddTextBox(" - " + pCloth->name);
    }

    if (!hasAnyClothing)
        db.AddTextBox("Your " + pItem->name + " is empty! Punch it while standing on it to store your outfit into it.");

    db.EndDialog("dressup_edit", "Take All Items", "Exit");
    pPlayer->SendOnDialogRequest(db.Get());
}

bool DressupDialog::RequestPunch(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile)
        return false;

    TileExtra_Dressup* pTileExtra = pTile->GetExtra<TileExtra_Dressup>();
    if (!pTileExtra)
        return false;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return false;

    if (!pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
    {
        pPlayer->SendOnTalkBubble("Only the owner can use this item.", false);
        return false;
    }

    if (!pWorld->GetTileManager()->GetTileParentTileWithWorldLock(pTile))
        return false;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem)
        return false;

    if (pItem->type != ITEM_TYPE_DRESSUP)
        return false;

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon("`w" + pItem->name + "``", pItem->id, true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y);

    string infoMsg = "Do you want to `4take off`` all your clothes and ";

    bool hasAnyClothing = false;
    for (int32 i = 0; i < 9; ++i)
    {
        if (pTileExtra->clothes[i] == ITEM_ID_BLANK)
            continue;

        if (!hasAnyClothing)
            infoMsg += "put on: ";

        hasAnyClothing = true;

        ItemInfo* pCloth = GetItemInfoManager()->GetItemByID(pTileExtra->clothes[i]);
        if (!pCloth)
            continue;

        infoMsg += "``[`2" + pCloth->name + "``] ";
    }

    if (!hasAnyClothing)
        infoMsg += "leave them inside " + pItem->name + "?";
    else
        infoMsg += "?";

    db.AddTextBox(infoMsg).EndDialog("dressup_ask", "Yes", "No");

    pPlayer->SendOnDialogRequest(db.Get());
    return true;
}

void DressupDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
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

    TileExtra_Dressup* pTileExtra = pTile->GetExtra<TileExtra_Dressup>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The dressup is gone!", false);
        return;
    }

    pPlayer->TryWearAllItemsFromDressup(pTile);
}

void DressupDialog::HandleAsk(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
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

    TileExtra_Dressup* pTileExtra = pTile->GetExtra<TileExtra_Dressup>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The dressup is gone!", false);
        return;
    }

    if (!pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
        return;

    PlayerInventory& inventory = pPlayer->GetInventory();
    uint32 invSpace = inventory.GetInventorySpace();

    for (int32 i = 0; i < 9; ++i)
    {
        int16 dressupItemID = pTileExtra->clothes[i];
        if (dressupItemID != ITEM_ID_BLANK)
        {
            ItemInfo* pCloth = GetItemInfoManager()->GetItemByID(dressupItemID);
            if (!pCloth || pCloth->HasFlag(ITEM_FLAG_UNTRADEABLE))
            {
                pPlayer->SendOnTalkBubble("Something went wrong.", false);
                return;
            }
        }

        int16 equippedItemID = inventory.GetClothByPart((eBodyPart)i);
        if (equippedItemID != ITEM_ID_BLANK)
        {
            ItemInfo* pCloth = GetItemInfoManager()->GetItemByID(equippedItemID);
            if (!pCloth || pCloth->HasFlag(ITEM_FLAG_UNTRADEABLE))
            {
                pPlayer->SendOnTalkBubble("Something went wrong.", false);
                return;
            }
        }

        if (dressupItemID != ITEM_ID_BLANK && dressupItemID != equippedItemID)
        {
            ItemInfo* pDressupItem = GetItemInfoManager()->GetItemByID(dressupItemID);
            if (!pDressupItem)
                return;

            uint32 currentCount = inventory.GetCountOfItem(pDressupItem->id);

            if (currentCount >= pDressupItem->maxCanHold)
            {
                pPlayer->SendOnTalkBubble("I can't hold any more " + pDressupItem->name + "!", false);
                return;
            }

            if (currentCount == 0)
                invSpace--;
        }

        if (equippedItemID != ITEM_ID_BLANK)
        {
            uint32 currentCount = inventory.GetCountOfItem(equippedItemID);
            if (currentCount == 1)
                invSpace++;

            ItemInfo* pEquippedItem = GetItemInfoManager()->GetItemByID(equippedItemID);
            if (!pEquippedItem)
                return;

            if (pEquippedItem->HasFlag(ITEM_FLAG_UNTRADEABLE))
            {
                pPlayer->SendOnTalkBubble("I can't drop my " + pEquippedItem->name, false);
                return;
            }
        }
    }

    if (invSpace < 0)
    {
        pPlayer->SendOnTalkBubble("I need more inventory space!", false);
        return;
    }

    string addedMsg;
    string removedMsg;

    for (int32 i = 0; i < 9; ++i)
    {
        int16 dressupItemID = pTileExtra->clothes[i];
        int16 equippedItemID = inventory.GetClothByPart((eBodyPart)i);

        if (dressupItemID == equippedItemID)
            continue;

        if (dressupItemID != ITEM_ID_BLANK)
        {
            ItemInfo* pDressupItem = GetItemInfoManager()->GetItemByID(dressupItemID);
            if (!pDressupItem)
                continue;

            if (!addedMsg.empty())
                addedMsg += ", ";
            addedMsg += pDressupItem->name;

            pPlayer->ModifyInventoryItem(pDressupItem->id, 1);
            pPlayer->ToggleCloth(pDressupItem->id);
        }

        if (equippedItemID != ITEM_ID_BLANK)
        {
            ItemInfo* pEquippedItem = GetItemInfoManager()->GetItemByID(equippedItemID);
            if (!pEquippedItem)
                continue;

            if (!addedMsg.empty())
                removedMsg += ", ";
            removedMsg += pEquippedItem->name;

            pPlayer->ModifyInventoryItem(pEquippedItem->id, -1);

            if (inventory.GetClothByPart((eBodyPart)i) == pEquippedItem->id)
                pPlayer->ToggleCloth(pEquippedItem->id);

            pTileExtra->SetCloth(i, pEquippedItem->id);
        }
        else
        {
            pTileExtra->SetCloth(i, ITEM_ID_BLANK);
        }
    }

    if (!addedMsg.empty() || !removedMsg.empty())
    {
        if (removedMsg.empty())
        {
            pPlayer->SendOnConsoleMessage("You put on [" + addedMsg + "]");
        }
        else
        {
            pPlayer->SendOnConsoleMessage("You put on [" + addedMsg + "] and removed [" + removedMsg + "]");
        }

        pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_SMOKE, pPlayer->GetWorldPosCenter(), 0, 1);
        pWorld->SendPlayPositionedToAll(pPlayer, "change_clothes.wav");
    }
}
