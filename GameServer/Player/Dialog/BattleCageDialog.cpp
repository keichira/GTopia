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
                bool hasPet2 = progressData.GetProgress(PLAYER_PROGRESS_PET_2_0) != ITEM_ID_BLANK;

                if(hasPet1 && hasPet2)
                {
                    db.AddTextBox("Your Battle Leash contains " + progressData.GetBattlePetName(0) + " and " + progressData.GetBattlePetName(1) + ".");
                }
                else if(hasPet1)
                {
                    db.AddTextBox("Your Battle Leash contains " + progressData.GetBattlePetName(0) + ".");
                }
                else
                {
                    db.AddTextBox("Your Battle Leash is empty.");
                }

                if(hasPet1)
                {
                    db.AddButton("dropoff1", "Cage your " + progressData.GetBattlePetName(0));
                }

                if(hasPet2)
                {
                    db.AddButton("dropoff2", "Cage your " + progressData.GetBattlePetName(1));
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
            case 0: randStatus += "sleeping in the back of the cage."; break;
            case 1: randStatus += "bares razor fangs at you in silence."; break;
            case 2: randStatus += "snarls and snaps as you get close."; break;
            case 3: randStatus += "eyes you hungrily."; break;
            case 4: randStatus += "stares at you, unblinking."; break;
        }

        db.AddTextBox(pPetInfo->GetColorCodeByElement() + pTileExtra->cageName + "`` the " + pPetInfo->name + "`` is " + randStatus);
        
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
            ->AddButton("pickitup", "Put " + pTileExtra->cageName + "`o in your Battle Leash")
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
        if(replace1 == ITEM_ID_BLANK && replace2 == ITEM_ID_BLANK)
        {
            if(critter != ITEM_ID_BLANK)
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
                    pPlayer->SendOnTalkBubble("A " + pPetItemInfo->name + " would not make a very good Battle Pet.", false);
                    return;
                }
    
                db.AddTextBox("Are you absolutely sure you want to `4permanently destroy`` your `2" + pItem->name + "`` to make a Battle Pet? Once caged, a Battle Pet can never be removed because it becomes too dangerous. Destroying the Battle Pet Cage will also destroy the pet inside.")
                ->EmbedData("critter", critter)
                ->AddButton("docage", "Yes, I am sure!")
                ->EndDialog("battlecage", "", "Nevermind");
            }
            else
            {
                auto pSetName = packet.Find("setname"_hash);
                if(!pSetName)
                    return;
    
                if(pSetName->size < 1 || pSetName->size > 20)
                    return;
    
                string setName(pSetName->value, pSetName->size);
                RemoveGTColorCodes(setName);
    
                if(setName.empty() || setName.size() > 20)
                    return;
    
                pTileExtra->cageName = setName;
                pWorld->SendTileUpdate(pTile);
    
                pPlayer->SendOnTalkBubble("You renamed your pet \"" + setName + "``\"!", false);
                return;
            }
        }
        else if(replace2 == ITEM_ID_BLANK || pTileExtra->secondPet != ITEM_ID_BLANK)
        {
            uint32 itemToUse = (replace2 != ITEM_ID_BLANK) ? replace2 : replace1;

            if(pTileExtra->basePet == itemToUse || pTileExtra->secondPet == itemToUse || pTileExtra->thirdPet == itemToUse)
            {
                pPlayer->SendOnTalkBubble("Your pet already has that ability!", false);
                return;
            }

            ItemInfo* pPetItemInfo = GetItemInfoManager()->GetItemByID(itemToUse);
            if(!pPetItemInfo)
                return;

            BattlePetInfo* pPetInfo = GetItemInfoManager()->GetBattlePetInfo(itemToUse);
            if(!pPetInfo)
            {
                pPlayer->SendOnTalkBubble("A " + pPetItemInfo->name + " has not Battle Pet abilities to splice.", false);
                return;
            }

            if(pPlayer->GetInventory().GetCountOfItem(ITEM_ID_WORLD_LOCK) < 10)
            {
                pPlayer->SendOnTalkBubble("You don't have 10 World Locks!", false);
                return;
            }

            BattlePetInfo* pBasePetInfo = GetItemInfoManager()->GetBattlePetInfo(pTileExtra->basePet);
            if(!pBasePetInfo)
                return;

            string costStr = " This will cost you `410 World Locks ``and the item you are gene-splicing.";

            if(pBasePetInfo->element != pPetInfo->element)
            {
                if(pPlayer->GetInventory().GetCountOfItem(ITEM_ID_CRISPR_TECHNOLOGY) < 1)
                {
                    pPlayer->SendOnTalkBubble("You can only splice creatures of the same element together without advanced technology!", false);
                    return;
                }

                costStr = " Because this pet is of a different element, it will cost you `410 World Locks``, `41 CRISPR Technology``, and the item you are gene-splicing.";
            }

            db.AddTextBox("Are you absolutely sure you want to `4permanently destroy`` your `2" + pPetItemInfo->name + "`` to gene-splice its ability into " + pTileExtra->cageName + "`o?" + costStr);

            int32 replacedItem = (replace2 != ITEM_ID_BLANK) ? pTileExtra->thirdPet : pTileExtra->secondPet;
            
            if(replacedItem != 0)
            {
                BattlePetInfo* pReplacedPetInfo = GetItemInfoManager()->GetBattlePetInfo(replacedItem);
                if(!pReplacedPetInfo)
                    return;

                db.AddTextBox("You will also lose your pet's existing " + pReplacedPetInfo->powerName);
            }

            string replaceButtonId = "replace";
            replaceButtonId += (replace2 != ITEM_ID_BLANK) ? "2" : "1";

            db.AddButton("doreplace", "Yes, I am sure")
            ->EmbedData(replaceButtonId, itemToUse)
            ->EndDialog("battlecage", "", "Nevermind");
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
                pWorld->SendConsoleMessageToAll(pPlayer->GetDisplayName(false) + "`o has caged "  + pTileExtra->cageName + " the " + pPetInfo->name);

                pWorld->SendTileUpdate(pTile);
                pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_TRANSFORM_FX, pTile->GetWorldPos());
                return;
            }

            case "resequence"_hash:
            {
                if(pPlayer->GetInventory().GetCountOfItem(ITEM_ID_GENETIC_RESEQUENCER) < 1)
                {
                    pPlayer->SendOnTalkBubble("You don't have Genetic Resequencer!", false);
                    return;
                }

                if(pTileExtra->secondPet == ITEM_ID_BLANK || pTileExtra->thirdPet == ITEM_ID_BLANK)
                {
                    pPlayer->SendOnTalkBubble("You need completely spliced pet to resequence its genes", false);
                    return;
                }

                db.AddTextBox("Resequencing " + pTileExtra->cageName + "'s genes will cost you 1 `5Genetic Resequencer``, and will leave it with the same 3 skills, but in a new order")
                ->AddTextBox("Are you sure you want to do this?")
                ->AddButton("doresequence", "Yes, I am sure!")
                ->EndDialog("battlecage", "", "Nevermind");
                break;
            }

            case "doresequence"_hash:
            {
                if(pPlayer->GetInventory().GetCountOfItem(ITEM_ID_GENETIC_RESEQUENCER) < 1)
                {
                    pPlayer->SendOnTalkBubble("You don't have a Genetic Resequencer!", false);
                    return;
                }

                if(pTileExtra->thirdPet == ITEM_ID_BLANK)
                {
                    pPlayer->SendOnTalkBubble("You need a completely spliced pet to resequence its genes.", false);
                    return;
                }

                int32 pet1 = pTileExtra->basePet;
                int32 pet2 = pTileExtra->secondPet;
                int32 pet3 = pTileExtra->thirdPet;

                pTileExtra->basePet = pet2;
                pTileExtra->secondPet = pet3;
                pTileExtra->thirdPet = pet1;

                pPlayer->SendOnTalkBubble(pTileExtra->cageName + " has resequenced " + GetFullBattlePetName(pet1, pet2, pet3) + "'s genes, it is now a " + GetFullBattlePetName(pet2, pet3, pet1) + ".", false);

                pPlayer->ModifyInventoryItem(ITEM_ID_GENETIC_RESEQUENCER, -1);
                pWorld->SendTileUpdate(pTile);
                pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_TRANSFORM_FX, pTile->GetWorldPos());
                return;
            }

            case "doreplace"_hash:
            {
                if(replace1 == ITEM_ID_BLANK && replace2 == ITEM_ID_BLANK)
                    return;

                uint32 itemToUse = (replace2 != ITEM_ID_BLANK) ? replace2 : replace1;
                if(pTileExtra->basePet == itemToUse || pTileExtra->secondPet == itemToUse || pTileExtra->thirdPet == itemToUse)
                {
                    pPlayer->SendOnTalkBubble("Your pet already has that ability!", false);
                    return;
                }
    
                ItemInfo* pPetItemInfo = GetItemInfoManager()->GetItemByID(itemToUse);
                if(!pPetItemInfo)
                    return;
    
                BattlePetInfo* pPetInfo = GetItemInfoManager()->GetBattlePetInfo(itemToUse);
                if(!pPetInfo)
                {
                    pPlayer->SendOnTalkBubble("A " + pPetItemInfo->name + " has not Battle Pet abilities to splice.", false);
                    return;
                }
    
                if(pPlayer->GetInventory().GetCountOfItem(ITEM_ID_WORLD_LOCK) < 10)
                {
                    pPlayer->SendOnTalkBubble("You don't have 10 World Locks!", false);
                    return;
                }

                if(pPlayer->GetInventory().GetCountOfItem(itemToUse) < 1)
                {
                    pPlayer->SendOnTalkBubble("You don't have that item", false);
                    return;
                }

                pPlayer->ModifyInventoryItem(itemToUse, -1);
                pPlayer->ModifyInventoryItem(ITEM_ID_WORLD_LOCK, -10);

                BattlePetInfo* pBasePetInfo = GetItemInfoManager()->GetBattlePetInfo(pTileExtra->basePet);
                if(!pBasePetInfo)
                    return;

                if(pBasePetInfo->element != pPetInfo->element)
                {
                    if(pPlayer->GetInventory().GetCountOfItem(ITEM_ID_CRISPR_TECHNOLOGY) < 1)
                    {
                        pPlayer->SendOnTalkBubble("You need CRISPR Technology!", false);
                        return;
                    }

                    pPlayer->ModifyInventoryItem(ITEM_ID_CRISPR_TECHNOLOGY, -1);
                }

                if(replace2 != ITEM_ID_BLANK)
                {
                    pTileExtra->thirdPet = itemToUse;
                }
                else if(replace2 == ITEM_ID_BLANK && replace1 != ITEM_ID_BLANK)
                {
                    pTileExtra->secondPet = itemToUse;
                }
                else
                    return;

                string notfiyMsg = "You spent 10 World Locks";
                if(pBasePetInfo->element != pPetInfo->element)
                {
                    notfiyMsg += ", 1 CRISPR Technology";
                }
                notfiyMsg += " and 1 " + pPetItemInfo->name + " to gene-splice the ability '" + pPetInfo->powerName + "' into your Battle Pet " + pTileExtra->cageName + "!";
                
                pPlayer->SendOnTalkBubble(notfiyMsg, false);
                pWorld->SendTileUpdate(pTile);
                pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_TRANSFORM_FX, pTile->GetWorldPos());
                return;
            }

            case "pickitup"_hash:
            {
                if(pTileExtra->thirdPet == ITEM_ID_BLANK)
                {
                    pPlayer->SendOnTalkBubble("A pet needs 3 powers to be put in a Battle Leash!", false);
                    return;
                }

                if(pPlayer->GetInventory().GetCountOfItem(ITEM_ID_BATTLE_LEASH) < 1)
                {
                    pPlayer->SendOnTalkBubble("You don't have a Battle Leash!", false);
                    return;
                }

                PlayerProgress& progressData = pPlayer->GetProgressData();
                if(progressData.GetProgress(PLAYER_PROGRESS_PET_1_0) != ITEM_ID_BLANK && progressData.GetProgress(PLAYER_PROGRESS_PET_2_0) != ITEM_ID_BLANK)
                {
                    pPlayer->SendOnTalkBubble("Your Battle Leash is full, take somebody out of it!", false);
                    return;
                }

                int32 freeSlotIdx = (progressData.GetProgress(PLAYER_PROGRESS_PET_1_0) == ITEM_ID_BLANK) ? PLAYER_PROGRESS_PET_1_0 : PLAYER_PROGRESS_PET_2_0;
                progressData.SetProgress((ePlayerProgress)freeSlotIdx, pTileExtra->basePet);
                progressData.SetProgress((ePlayerProgress)(freeSlotIdx + 1), pTileExtra->secondPet);
                progressData.SetProgress((ePlayerProgress)(freeSlotIdx + 2), pTileExtra->thirdPet);

                pTileExtra->basePet = ITEM_ID_BLANK;
                pTileExtra->secondPet = ITEM_ID_BLANK;
                pTileExtra->thirdPet = ITEM_ID_BLANK;

                pPlayer->SendOnTalkBubble("I stuffed " + pTileExtra->cageName + " the " + progressData.GetBattlePetName(freeSlotIdx) + " into my Battle Leash!", false);
                pWorld->SendConsoleMessageToAll(pPlayer->GetDisplayName(false) + "`o put " + pTileExtra->cageName + " the " + progressData.GetBattlePetName(freeSlotIdx) + " in their Battle Leash");
                pWorld->SendTileUpdate(pTile);

                pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_TRANSFORM_FX, pTile->GetWorldPosCenter());

                if(pPlayer->GetInventory().GetClothByPart(BODY_PART_HAND) == ITEM_ID_BATTLE_LEASH)
                {
                    pPlayer->ToggleBattlePetLeash(true);
                }

                return;
            }

            case "dropoff1"_hash:
            {
                if(pTileExtra->basePet != ITEM_ID_BLANK)
                    return;

                PlayerProgress& progressData = pPlayer->GetProgressData();
                if(progressData.GetProgress(PLAYER_PROGRESS_PET_1_0) == ITEM_ID_BLANK)
                    return;

                pTileExtra->basePet = progressData.GetProgress(PLAYER_PROGRESS_PET_1_0);
                pTileExtra->secondPet = progressData.GetProgress(PLAYER_PROGRESS_PET_1_1);
                pTileExtra->thirdPet = progressData.GetProgress(PLAYER_PROGRESS_PET_1_2);

                progressData.SetProgress(PLAYER_PROGRESS_PET_1_0, ITEM_ID_BLANK);
                progressData.SetProgress(PLAYER_PROGRESS_PET_1_1, ITEM_ID_BLANK);
                progressData.SetProgress(PLAYER_PROGRESS_PET_1_2, ITEM_ID_BLANK);

                if(pTileExtra->cageName.empty())
                {
                    pTileExtra->cageName = GetRandomGrowNamePart() + GetRandomGrowNamePart();
                }

                pWorld->SendTileUpdate(pTile);
                pPlayer->ToggleBattlePetLeash(true);

                pPlayer->SendOnTalkBubble("I caged " + pTileExtra->cageName + " the " + progressData.GetBattlePetName(0), false);
                pWorld->SendConsoleMessageToAll(pPlayer->GetDisplayName(false) + "`o caged " + pTileExtra->cageName + " the " + progressData.GetBattlePetName(0));

                pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_TRANSFORM_FX, pTile->GetWorldPosCenter());
                return;
            }

            case "dropoff2"_hash:
            {
                if(pTileExtra->basePet != ITEM_ID_BLANK)
                    return;

                PlayerProgress& progressData = pPlayer->GetProgressData();
                if(progressData.GetProgress(PLAYER_PROGRESS_PET_2_0) == ITEM_ID_BLANK)
                    return;

                pTileExtra->basePet = progressData.GetProgress(PLAYER_PROGRESS_PET_2_0);
                pTileExtra->secondPet = progressData.GetProgress(PLAYER_PROGRESS_PET_2_1);
                pTileExtra->thirdPet = progressData.GetProgress(PLAYER_PROGRESS_PET_2_2);

                progressData.SetProgress(PLAYER_PROGRESS_PET_2_0, ITEM_ID_BLANK);
                progressData.SetProgress(PLAYER_PROGRESS_PET_2_1, ITEM_ID_BLANK);
                progressData.SetProgress(PLAYER_PROGRESS_PET_2_2, ITEM_ID_BLANK);

                if(pTileExtra->cageName.empty())
                {
                    pTileExtra->cageName = GetRandomGrowNamePart() + GetRandomGrowNamePart();
                }

                pWorld->SendTileUpdate(pTile);
                if(pPlayer->GetActiveBattlePetSlot() == 1)
                {
                    pPlayer->ToggleBattlePetLeash(false);
                }

                pPlayer->SendOnTalkBubble("I caged " + pTileExtra->cageName + " the " + progressData.GetBattlePetName(1), false);
                pWorld->SendConsoleMessageToAll(pPlayer->GetDisplayName(false) + "`o caged " + pTileExtra->cageName + " the " + progressData.GetBattlePetName(1));

                pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_TRANSFORM_FX, pTile->GetWorldPosCenter());
                return;
            }

            default:
                return;
        }
    }

    pPlayer->SendOnDialogRequest(db.Get());
}
