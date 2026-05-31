#include "BattleCageDialog.h"
#include "Item/ItemInfoManager.h"
#include "../../World/WorldManager.h"
#include "../GamePlayer.h"
#include "Utils/DialogBuilder.h"
#include "Utils/GrowUtils.h"

void BattleCageDialog::Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem)
{
    if(!pPlayer || !pTile || !pItem)
        return;

    if(pItem->type != ITEM_TYPE_BATTLE_CAGE)
        return;

    TileExtra_BattleCage* pTileExtra = pTile->GetExtra<TileExtra_BattleCage>();
    if(!pTileExtra)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    bool hasAccessToEdit = pWorld->PlayerHasAccessOnTile(pPlayer, pTile);

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
    ->AddLabelWithIcon(pItem->name, pItem->id, true)
    ->EmbedData("tilex", vTilePos.x)
    ->EmbedData("tiley", vTilePos.y);

    PlayerProgress& progressData = pPlayer->GetProgressData();

    if(pTileExtra->basePet == ITEM_ID_BLANK)
    {
        if(pTileExtra->cageName.empty())
        {
            db.AddTextBox("This cage is empty.");
        }
        else
        {
            db.AddTextBox("This cage is empty, but labeled \"" + pTileExtra->cageName + "\".");
        }

        if(hasAccessToEdit)
        {
            db.AddItemPicker("critter", "`wCage Beast``", "Select any pet to cage.");

            if(pPlayer->GetInventory().GetClothByPart(BODY_PART_HAND) == ITEM_ID_BATTLE_LEASH)
            {
                bool hasPet1 = progressData.GetProgress(PLAYER_PROGRESS_PET_1_0) != ITEM_ID_BLANK;
                bool hasPet2 = progressData.GetProgress(PLAYER_PROGRESS_PET_1_0) != ITEM_ID_BLANK;

                if(hasPet1 && hasPet2)
                {

                }
                else if(hasPet1)
                {

                }
                else
                {
                    db.AddTextBox("Your Battle Leash is empty.");
                }
            }

            db.EndDialog("battlecage", "", "Cancel");
        }
    }
    else
    {
        BattlePetInfo* pPetInfo = GetItemInfoManager()->GetBattlePetInfo(pTileExtra->basePet);
        if(!pPetInfo)
            return;

        string randStatus;
        switch(RandomRangeInt(0, 4))
        {
            case 0: randStatus += "is sleeping in the back of the cage."; break;
            case 1: randStatus += "bares razor fangs at you in silence."; break;
            case 2: randStatus += "snarls and snaps as you get close."; break;
            case 3: randStatus += "eyes you hungrily."; break;
            case 4: randStatus += "stares at you, unblinking."; break;
        }

        db.AddTextBox(pPetInfo->GetColorCodeByElement() + pTileExtra->cageName + " the " + pPetInfo->name + "`` is " + randStatus);
        
        if(!hasAccessToEdit)
        {

            db.EndDialog("battlecage", "", "Back away slowly");
        }
        else
        {
            db.AddSmallText("`wNatural ability:``")
            ->AddLabelWithIcon(pPetInfo->GetDescribedPower(), pPetInfo->itemID);

            if(pTileExtra->secondPet != ITEM_ID_BLANK)
            {
                BattlePetInfo* pPetInfo2 = GetItemInfoManager()->GetBattlePetInfo(pTileExtra->secondPet);

                if(pPetInfo2)
                {
                    db.AddSmallText("`wGene-spliced ability:``")
                    ->AddLabelWithIcon(pPetInfo2->GetDescribedPower(), pPetInfo2->itemID)
                    ->AddItemPicker("replace1", "Replace this ability", "Select any pet to splice its genes");
                }

                if(pTileExtra->thirdPet != ITEM_ID_BLANK)
                {
                    BattlePetInfo* pPetInfo3 = GetItemInfoManager()->GetBattlePetInfo(pTileExtra->thirdPet);
    
                    if(pPetInfo3)
                    {
                        db.AddSmallText("`wGene-spliced ability:``")
                        ->AddLabelWithIcon(pPetInfo3->GetDescribedPower(), pPetInfo3->itemID)
                        ->AddItemPicker("replace2", "Replace this ability", "Select any pet to splice its genes");
                    }

                    if(pPlayer->GetInventory().GetCountOfItem(ITEM_ID_GENETIC_RESEQUENCER) > 0)
                    {
                        db.AddButton("resequence", "Resequence this pet's genome");
                    }
                }
                else
                {
                    db.AddItemPicker("replace2", "Gene-splice a new ability", "Select any pet to splice its genes");
                }
            }
            else
            {
                db.AddItemPicker("replace1", "Gene-splice a new ability", "Select any pet to splice its genes");
            }

            db.AddSmallText("It costs 10 World Locks to splice in the genes from another creature and add or replace an ability. A Battle Pet needs 3 Abilities to be used in battles.")
            ->AddButton("pickitup", "Put " + pTileExtra->cageName + "`` in your Battle Leash")
            ->AddTextInput("setname", "Name:", pTileExtra->cageName, 20)
            ->EndDialog("battlecage", "Rename", "Cancel");
        }
    }

    pPlayer->SendOnDialogRequest(db.Get());
}

void BattleCageDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
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

    if(!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        return;

    TileExtra_BattleCage* pTileExtra = pTile->GetExtra<TileExtra_BattleCage>();
    if(!pTileExtra)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetDisplayedItem());
    if(!pItem || pItem->type != ITEM_TYPE_BATTLE_CAGE)
        return;

    uint32 critter = 0;
    uint32 replace1 = 0;
    uint32 replace2 = 0;

    auto pCritter = packet.Find("critter"_hash);
    if(pCritter)
    {
        if(ToUInt(string(pCritter->value, pCritter->size), critter) != TO_INT_SUCCESS)
            return;
    }

    auto pReplace1 = packet.Find("replace1"_hash);
    if(pReplace1)
    {
        if(ToUInt(string(pReplace1->value, pReplace1->size), replace1) != TO_INT_SUCCESS)
            return;
    }

    auto pReplace2 = packet.Find("replace2"_hash);
    if(pReplace2)
    {
        if(ToUInt(string(pReplace2->value, pReplace2->size), replace2) != TO_INT_SUCCESS)
            return;
    }

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
    ->AddLabelWithIcon(pItem->name, pItem->id, true)
    ->EmbedData("tilex", vTilePos.x)
    ->EmbedData("tiley", vTilePos.y);

    auto pButtonClicked = packet.Find("buttonClicked"_hash);
    if(!pButtonClicked)
    {
        if(replace1 == ITEM_ID_BLANK && replace2 == ITEM_ID_BLANK && critter != ITEM_ID_BLANK)
        {
            // start
            if(pTileExtra->basePet != ITEM_ID_BLANK)
            {
                pPlayer->SendOnTalkBubble("Something already lives in there!", false);
                return;
            }

            ItemInfo* pPetItemInfo = GetItemInfoManager()->GetItemByID(critter);
            if(!pPetItemInfo)
                return;

            BattlePetInfo* pPetInfo = GetItemInfoManager()->GetBattlePetInfo(critter);
            if(!pPetInfo)
            {
                pPlayer->SendOnTalkBubble("A " + pPetInfo->name + " would not make a very good Battle Pet.", false);
                return;
            }

            db.AddTextBox("Are you absolutely sure you want to `4permanently destroy`` your `2" + pItem->name + "`` to make a Battle Pet? Once caged, a Battle Pet can never be removed because it becomes too dangerous. Destroying the Battle Pet Cage will also destroy the pet inside.")
            ->EmbedData("critter", critter)
            ->AddButton("docage", "Yes, I am sure!")
            ->EndDialog("battlecage", "", "Nevermind");
        }
        else if(replace1 == ITEM_ID_BLANK && replace2 == ITEM_ID_BLANK && critter == ITEM_ID_BLANK)
        {
            auto pSetName = packet.Find("setname"_hash);
            if(!pSetName)
                return;

            if(pSetName->size < 1 || pSetName->size > 20)
                return;

            string setName(pSetName->value, pSetName->size);
            if(setName.empty() || setName.size() > 20)
                return;

            pTileExtra->cageName = setName;
            pWorld->SendTileUpdate(pTile);

            pPlayer->SendOnTalkBubble("You renamed your pet \"" + setName + "``\"!", false);
            return;
        }
    }
    else
    {
        if(pButtonClicked->size < 1 || pButtonClicked->size > 25)
            return;

        uint32 buttonClickedHash = HashString(pButtonClicked->value, pButtonClicked->size);

        switch(buttonClickedHash)
        {
            case "docage"_hash:
            {
                if(critter == ITEM_ID_BLANK)
                    return;

                if(pTileExtra->basePet != ITEM_ID_BLANK)
                {
                    pPlayer->SendOnTalkBubble("Something already lives in there!", false);
                    return;
                }

                ItemInfo* pPetItemInfo = GetItemInfoManager()->GetItemByID(critter);
                if(!pPetItemInfo)
                    return;
    
                BattlePetInfo* pPetInfo = GetItemInfoManager()->GetBattlePetInfo(critter);
                if(!pPetInfo)
                {
                    pPlayer->SendOnTalkBubble("A " + pPetInfo->name + " would not make a very good Battle Pet.", false);
                    return;
                }

                if(pPlayer->GetInventory().GetCountOfItem(critter) < 1)
                {
                    pPlayer->SendOnTalkBubble("You don't have that item!", false);
                    return;
                }

                pPlayer->ModifyInventoryItem(critter, -1);
                pTileExtra->basePet = critter;
                pTileExtra->cageName = GetRandomGrowNamePart() + GetRandomGrowNamePart();

                pPlayer->SendOnTalkBubble("I have caged " + pTileExtra->cageName + " the " + pPetInfo->name, false);
                pWorld->SendConsoleMessageToAll(pPlayer->GetDisplayName(true) + " has caged "  + pTileExtra->cageName + " the " + pPetInfo->name);

                pWorld->SendTileUpdate(pTile);
                pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_TRANSFORM_FX, pTile->GetWorldPos());
                return;
            }
        }
    }

    pPlayer->SendOnDialogRequest(db.Get());
}
