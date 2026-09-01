#include "GameDialogs.h"
#include "../Context.h"
#include "../Item/HarmonicCrystal.h"
#include "../Item/SuckerBlockManager.h"
#include "../Player/GamePlayer.h"
#include "../Player/PlayerManager.h"
#include "../Server/MasterBroadway.h"
#include "../Server/UserCacheManager.h"
#include "../World/WorldManager.h"
#include "Utils/GrowUtils.h"
#include "Utils/StringUtils.h"

void AchievementBlockDialog::Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem)
{
    if (!pPlayer || !pTile || !pItem)
        return;

    if (pItem->type != ITEM_TYPE_ACHIEVEMENT)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    if (pWorld->GetWorldOwnerID() != pPlayer->GetUserID())
    {
        pPlayer->SendOnTalkBubble(
            "An `5Achievement Block`` can only be etched by the owner of this world. (Requires `5World Lock``)", false);
        return;
    }

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.AddLabelWithIcon("`w" + pItem->name + "``", pItem->id, true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y)
        .AddTextBox("Which design do you want to etch into the block? (Tap the icon)");

    if (pPlayer->GetProgressData().BuildAchievementsDialog(db, true) == 0)
    {
        db.AddTextBox("(`4Oops, you haven't earned any achievements yet, come back later!``)");
    }

    db.EndDialog("achieve_reply", "Clear It", "Cancel");
    pPlayer->SendOnDialogRequest(db.Get());
}

void AchievementBlockDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
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

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetDisplayedItem());
    if (!pItem || pItem->type != ITEM_TYPE_ACHIEVEMENT)
        return;

    if (pPlayer->GetUserID() != pWorld->GetWorldOwnerID())
    {
        pPlayer->SendOnTalkBubble("Only the owner of the `$World Lock`` can etch these blocks.", true);
        return;
    }

    TileExtra_Achievement* pTileExtra = pTile->GetExtra<TileExtra_Achievement>();
    if (!pTileExtra)
        return;

    int32 achievementID = 127;
    if (auto pButtonClicked = packet.Find("buttonClicked"_hash))
    {
        if (pButtonClicked->GetInt(achievementID) != TO_INT_SUCCESS)
            return;
    }

    if (achievementID != 127 && achievementID < 0 && achievementID > ACHIEVEMENT_COUNT &&
        !pPlayer->GetProgressData().HasAchievement((eAchievement)achievementID))
        return;

    string message = "Block etched.";
    if (achievementID == 127)
    {
        message = "`5Achievement Block`` cleared.";
    }

    pTileExtra->achievementID = achievementID;
    pWorld->SendTileUpdate(pTile);
    pPlayer->SendOnTalkBubble(message, false);
}

void BattleCageDialog::Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem)
{
    if (!pPlayer || !pTile || !pItem)
        return;

    if (pItem->type != ITEM_TYPE_BATTLE_CAGE)
        return;

    TileExtra_BattleCage* pTileExtra = pTile->GetExtra<TileExtra_BattleCage>();
    if (!pTileExtra)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    bool hasAccessToEdit = pWorld->PlayerHasAccessOnTile(pPlayer, pTile);

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon(pItem->name, pItem->id, true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y);

    PlayerProgress& progressData = pPlayer->GetProgressData();

    if (pTileExtra->basePet == ITEM_ID_BLANK)
    {
        if (pTileExtra->cageName.empty())
        {
            db.AddTextBox("This cage is empty.");
        }
        else
        {
            db.AddTextBox("This cage is empty, but labeled \"" + pTileExtra->cageName + "\".");
        }

        if (hasAccessToEdit)
        {
            db.AddItemPicker("critter", "`wCage Beast``", "Select any pet to cage.");

            if (pPlayer->GetInventory().GetClothByPart(BODY_PART_HAND) == ITEM_ID_BATTLE_LEASH)
            {
                bool hasPet1 = progressData.GetProgress(PLAYER_PROGRESS_PET_1_0) != ITEM_ID_BLANK;
                bool hasPet2 = progressData.GetProgress(PLAYER_PROGRESS_PET_2_0) != ITEM_ID_BLANK;

                if (hasPet1 && hasPet2)
                {
                    db.AddTextBox("Your Battle Leash contains " + progressData.GetBattlePetName(0) + " and " +
                                  progressData.GetBattlePetName(1) + ".");
                }
                else if (hasPet1)
                {
                    db.AddTextBox("Your Battle Leash contains " + progressData.GetBattlePetName(0) + ".");
                }
                else
                {
                    db.AddTextBox("Your Battle Leash is empty.");
                }

                if (hasPet1)
                {
                    db.AddButton("dropoff1", "Cage your " + progressData.GetBattlePetName(0));
                }

                if (hasPet2)
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
        if (!pPetInfo)
            return;

        string randStatus;
        switch (RandomRangeInt(0, 4))
        {
            case 0:
                randStatus += "sleeping in the back of the cage.";
                break;
            case 1:
                randStatus += "bares razor fangs at you in silence.";
                break;
            case 2:
                randStatus += "snarls and snaps as you get close.";
                break;
            case 3:
                randStatus += "eyes you hungrily.";
                break;
            case 4:
                randStatus += "stares at you, unblinking.";
                break;
        }

        db.AddTextBox(pPetInfo->GetColorCodeByElement() + pTileExtra->cageName + "`` the " + pPetInfo->name + "`` is " +
                      randStatus);

        if (!hasAccessToEdit)
        {
            db.EndDialog("battlecage", "", "Back away slowly");
        }
        else
        {
            db.AddSmallText("`wNatural ability:``").AddLabelWithIcon(pPetInfo->GetDescribedPower(), pPetInfo->itemID);

            if (pTileExtra->secondPet != ITEM_ID_BLANK)
            {
                BattlePetInfo* pPetInfo2 = GetItemInfoManager()->GetBattlePetInfo(pTileExtra->secondPet);

                if (pPetInfo2)
                {
                    db.AddSmallText("`wGene-spliced ability:``")
                        .AddLabelWithIcon(pPetInfo2->GetDescribedPower(), pPetInfo2->itemID)
                        .AddItemPicker("replace1", "Replace this ability", "Select any pet to splice its genes");
                }

                if (pTileExtra->thirdPet != ITEM_ID_BLANK)
                {
                    BattlePetInfo* pPetInfo3 = GetItemInfoManager()->GetBattlePetInfo(pTileExtra->thirdPet);

                    if (pPetInfo3)
                    {
                        db.AddSmallText("`wGene-spliced ability:``")
                            .AddLabelWithIcon(pPetInfo3->GetDescribedPower(), pPetInfo3->itemID)
                            .AddItemPicker("replace2", "Replace this ability", "Select any pet to splice its genes");
                    }

                    if (pPlayer->GetInventory().GetCountOfItem(ITEM_ID_GENETIC_RESEQUENCER) > 0)
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

            db.AddSmallText("It costs 10 World Locks to splice in the genes from another creature and add or replace "
                            "an ability. A Battle Pet needs 3 Abilities to be used in battles.")
                .AddButton("pickitup", "Put " + pTileExtra->cageName + "`o in your Battle Leash")
                .AddTextInput("setname", "Name:", pTileExtra->cageName, 20)
                .EndDialog("battlecage", "Rename", "Cancel");
        }
    }

    pPlayer->SendOnDialogRequest(db.Get());
}

void BattleCageDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
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

    if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        return;

    TileExtra_BattleCage* pTileExtra = pTile->GetExtra<TileExtra_BattleCage>();
    if (!pTileExtra)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetDisplayedItem());
    if (!pItem || pItem->type != ITEM_TYPE_BATTLE_CAGE)
        return;

    uint32 critter = 0;
    uint32 replace1 = 0;
    uint32 replace2 = 0;

    auto pCritter = packet.Find("critter"_hash);
    if (pCritter)
    {
        if (pCritter->GetUInt(critter) != TO_INT_SUCCESS)
            return;
    }

    auto pReplace1 = packet.Find("replace1"_hash);
    if (pReplace1)
    {
        if (pReplace1->GetUInt(replace1) != TO_INT_SUCCESS)
            return;
    }

    auto pReplace2 = packet.Find("replace2"_hash);
    if (pReplace2)
    {
        if (pReplace2->GetUInt(replace2) != TO_INT_SUCCESS)
            return;
    }

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon(pItem->name, pItem->id, true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y);

    auto pButtonClicked = packet.Find("buttonClicked"_hash);
    if (!pButtonClicked)
    {
        if (replace1 == ITEM_ID_BLANK && replace2 == ITEM_ID_BLANK)
        {
            if (critter != ITEM_ID_BLANK)
            {
                // start
                if (pTileExtra->basePet != ITEM_ID_BLANK)
                {
                    pPlayer->SendOnTalkBubble("Something already lives in there!", false);
                    return;
                }

                ItemInfo* pPetItemInfo = GetItemInfoManager()->GetItemByID(critter);
                if (!pPetItemInfo)
                    return;

                BattlePetInfo* pPetInfo = GetItemInfoManager()->GetBattlePetInfo(critter);
                if (!pPetInfo)
                {
                    pPlayer->SendOnTalkBubble("A " + pPetItemInfo->name + " would not make a very good Battle Pet.",
                                              false);
                    return;
                }

                db.AddTextBox("Are you absolutely sure you want to `4permanently destroy`` your `2" + pItem->name +
                              "`` to make a Battle Pet? Once caged, a Battle Pet can never be removed because it "
                              "becomes too dangerous. Destroying the Battle Pet Cage will also destroy the pet inside.")
                    .EmbedData("critter", critter)
                    .AddButton("docage", "Yes, I am sure!")
                    .EndDialog("battlecage", "", "Nevermind");
            }
            else
            {
                auto pSetName = packet.Find("setname"_hash);
                if (!pSetName)
                    return;

                if (pSetName->valueSize < 1 || pSetName->valueSize > 20)
                    return;

                string setName = pSetName->GetString();
                RemoveGTColorCodes(setName);

                if (setName.empty() || setName.size() > 20)
                    return;

                pTileExtra->cageName = setName;
                pWorld->SendTileUpdate(pTile);

                pPlayer->SendOnTalkBubble("You renamed your pet \"" + setName + "``\"!", false);
                return;
            }
        }
        else if (replace2 == ITEM_ID_BLANK || pTileExtra->secondPet != ITEM_ID_BLANK)
        {
            uint32 itemToUse = (replace2 != ITEM_ID_BLANK) ? replace2 : replace1;

            if (pTileExtra->basePet == itemToUse || pTileExtra->secondPet == itemToUse ||
                pTileExtra->thirdPet == itemToUse)
            {
                pPlayer->SendOnTalkBubble("Your pet already has that ability!", false);
                return;
            }

            ItemInfo* pPetItemInfo = GetItemInfoManager()->GetItemByID(itemToUse);
            if (!pPetItemInfo)
                return;

            BattlePetInfo* pPetInfo = GetItemInfoManager()->GetBattlePetInfo(itemToUse);
            if (!pPetInfo)
            {
                pPlayer->SendOnTalkBubble("A " + pPetItemInfo->name + " has not Battle Pet abilities to splice.",
                                          false);
                return;
            }

            if (pPlayer->GetInventory().GetCountOfItem(ITEM_ID_WORLD_LOCK) < 10)
            {
                pPlayer->SendOnTalkBubble("You don't have 10 World Locks!", false);
                return;
            }

            BattlePetInfo* pBasePetInfo = GetItemInfoManager()->GetBattlePetInfo(pTileExtra->basePet);
            if (!pBasePetInfo)
                return;

            string costStr = " This will cost you `410 World Locks ``and the item you are gene-splicing.";

            if (pBasePetInfo->element != pPetInfo->element)
            {
                if (pPlayer->GetInventory().GetCountOfItem(ITEM_ID_CRISPR_TECHNOLOGY) < 1)
                {
                    pPlayer->SendOnTalkBubble(
                        "You can only splice creatures of the same element together without advanced technology!",
                        false);
                    return;
                }

                costStr = " Because this pet is of a different element, it will cost you `410 World Locks``, `41 "
                          "CRISPR Technology``, and the item you are gene-splicing.";
            }

            db.AddTextBox("Are you absolutely sure you want to `4permanently destroy`` your `2" + pPetItemInfo->name +
                          "`` to gene-splice its ability into " + pTileExtra->cageName + "`o?" + costStr);

            int32 replacedItem = (replace2 != ITEM_ID_BLANK) ? pTileExtra->thirdPet : pTileExtra->secondPet;

            if (replacedItem != 0)
            {
                BattlePetInfo* pReplacedPetInfo = GetItemInfoManager()->GetBattlePetInfo(replacedItem);
                if (!pReplacedPetInfo)
                    return;

                db.AddTextBox("You will also lose your pet's existing " + pReplacedPetInfo->powerName);
            }

            string replaceButtonId = "replace";
            replaceButtonId += (replace2 != ITEM_ID_BLANK) ? "2" : "1";

            db.AddButton("doreplace", "Yes, I am sure")
                .EmbedData(replaceButtonId, itemToUse)
                .EndDialog("battlecage", "", "Nevermind");
        }
    }
    else
    {
        if (pButtonClicked->valueSize == 0 || pButtonClicked->valueSize > 25)
            return;

        uint32 buttonClickedHash = HashString(pButtonClicked->value, pButtonClicked->valueSize);

        switch (buttonClickedHash)
        {
            case "docage"_hash:
            {
                if (critter == ITEM_ID_BLANK)
                    return;

                if (pTileExtra->basePet != ITEM_ID_BLANK)
                {
                    pPlayer->SendOnTalkBubble("Something already lives in there!", false);
                    return;
                }

                ItemInfo* pPetItemInfo = GetItemInfoManager()->GetItemByID(critter);
                if (!pPetItemInfo)
                    return;

                BattlePetInfo* pPetInfo = GetItemInfoManager()->GetBattlePetInfo(critter);
                if (!pPetInfo)
                {
                    pPlayer->SendOnTalkBubble("A " + pPetInfo->name + " would not make a very good Battle Pet.", false);
                    return;
                }

                if (pPlayer->GetInventory().GetCountOfItem(critter) < 1)
                {
                    pPlayer->SendOnTalkBubble("You don't have that item!", false);
                    return;
                }

                pPlayer->ModifyInventoryItem(critter, -1);
                pTileExtra->basePet = critter;
                pTileExtra->cageName = GetRandomGrowNamePart() + GetRandomGrowNamePart();

                pPlayer->SendOnTalkBubble("I have caged " + pTileExtra->cageName + " the " + pPetInfo->name, false);
                pWorld->SendConsoleMessageToAll(pPlayer->GetDisplayName(false) + "`o has caged " +
                                                pTileExtra->cageName + " the " + pPetInfo->name);

                pWorld->SendTileUpdate(pTile);
                pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_TRANSFORM_FX, pTile->GetWorldPos());
                return;
            }

            case "resequence"_hash:
            {
                if (pPlayer->GetInventory().GetCountOfItem(ITEM_ID_GENETIC_RESEQUENCER) < 1)
                {
                    pPlayer->SendOnTalkBubble("You don't have Genetic Resequencer!", false);
                    return;
                }

                if (pTileExtra->secondPet == ITEM_ID_BLANK || pTileExtra->thirdPet == ITEM_ID_BLANK)
                {
                    pPlayer->SendOnTalkBubble("You need completely spliced pet to resequence its genes", false);
                    return;
                }

                db.AddTextBox("Resequencing " + pTileExtra->cageName +
                              "'s genes will cost you 1 `5Genetic Resequencer``, and will leave it with the same 3 "
                              "skills, but in a new order")
                    .AddTextBox("Are you sure you want to do this?")
                    .AddButton("doresequence", "Yes, I am sure!")
                    .EndDialog("battlecage", "", "Nevermind");
                break;
            }

            case "doresequence"_hash:
            {
                if (pPlayer->GetInventory().GetCountOfItem(ITEM_ID_GENETIC_RESEQUENCER) < 1)
                {
                    pPlayer->SendOnTalkBubble("You don't have a Genetic Resequencer!", false);
                    return;
                }

                if (pTileExtra->thirdPet == ITEM_ID_BLANK)
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

                pPlayer->SendOnTalkBubble(pTileExtra->cageName + " has resequenced " +
                                              GetFullBattlePetName(pet1, pet2, pet3) + "'s genes, it is now a " +
                                              GetFullBattlePetName(pet2, pet3, pet1) + ".",
                                          false);

                pPlayer->ModifyInventoryItem(ITEM_ID_GENETIC_RESEQUENCER, -1);
                pWorld->SendTileUpdate(pTile);
                pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_TRANSFORM_FX, pTile->GetWorldPos());
                return;
            }

            case "doreplace"_hash:
            {
                if (replace1 == ITEM_ID_BLANK && replace2 == ITEM_ID_BLANK)
                    return;

                uint32 itemToUse = (replace2 != ITEM_ID_BLANK) ? replace2 : replace1;
                if (pTileExtra->basePet == itemToUse || pTileExtra->secondPet == itemToUse ||
                    pTileExtra->thirdPet == itemToUse)
                {
                    pPlayer->SendOnTalkBubble("Your pet already has that ability!", false);
                    return;
                }

                ItemInfo* pPetItemInfo = GetItemInfoManager()->GetItemByID(itemToUse);
                if (!pPetItemInfo)
                    return;

                BattlePetInfo* pPetInfo = GetItemInfoManager()->GetBattlePetInfo(itemToUse);
                if (!pPetInfo)
                {
                    pPlayer->SendOnTalkBubble("A " + pPetItemInfo->name + " has not Battle Pet abilities to splice.",
                                              false);
                    return;
                }

                if (pPlayer->GetInventory().GetCountOfItem(ITEM_ID_WORLD_LOCK) < 10)
                {
                    pPlayer->SendOnTalkBubble("You don't have 10 World Locks!", false);
                    return;
                }

                if (pPlayer->GetInventory().GetCountOfItem(itemToUse) < 1)
                {
                    pPlayer->SendOnTalkBubble("You don't have that item", false);
                    return;
                }

                pPlayer->ModifyInventoryItem(itemToUse, -1);
                pPlayer->ModifyInventoryItem(ITEM_ID_WORLD_LOCK, -10);

                BattlePetInfo* pBasePetInfo = GetItemInfoManager()->GetBattlePetInfo(pTileExtra->basePet);
                if (!pBasePetInfo)
                    return;

                if (pBasePetInfo->element != pPetInfo->element)
                {
                    if (pPlayer->GetInventory().GetCountOfItem(ITEM_ID_CRISPR_TECHNOLOGY) < 1)
                    {
                        pPlayer->SendOnTalkBubble("You need CRISPR Technology!", false);
                        return;
                    }

                    pPlayer->ModifyInventoryItem(ITEM_ID_CRISPR_TECHNOLOGY, -1);
                }

                if (replace2 != ITEM_ID_BLANK)
                {
                    pTileExtra->thirdPet = itemToUse;
                }
                else if (replace2 == ITEM_ID_BLANK && replace1 != ITEM_ID_BLANK)
                {
                    pTileExtra->secondPet = itemToUse;
                }
                else
                    return;

                string notfiyMsg = "You spent 10 World Locks";
                if (pBasePetInfo->element != pPetInfo->element)
                {
                    notfiyMsg += ", 1 CRISPR Technology";
                }
                notfiyMsg += " and 1 " + pPetItemInfo->name + " to gene-splice the ability '" + pPetInfo->powerName +
                             "' into your Battle Pet " + pTileExtra->cageName + "!";

                pPlayer->SendOnTalkBubble(notfiyMsg, false);
                pWorld->SendTileUpdate(pTile);
                pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_TRANSFORM_FX, pTile->GetWorldPos());
                return;
            }

            case "pickitup"_hash:
            {
                if (pTileExtra->thirdPet == ITEM_ID_BLANK)
                {
                    pPlayer->SendOnTalkBubble("A pet needs 3 powers to be put in a Battle Leash!", false);
                    return;
                }

                if (pPlayer->GetInventory().GetCountOfItem(ITEM_ID_BATTLE_LEASH) < 1)
                {
                    pPlayer->SendOnTalkBubble("You don't have a Battle Leash!", false);
                    return;
                }

                PlayerProgress& progressData = pPlayer->GetProgressData();
                if (progressData.GetProgress(PLAYER_PROGRESS_PET_1_0) != ITEM_ID_BLANK &&
                    progressData.GetProgress(PLAYER_PROGRESS_PET_2_0) != ITEM_ID_BLANK)
                {
                    pPlayer->SendOnTalkBubble("Your Battle Leash is full, take somebody out of it!", false);
                    return;
                }

                int32 freeSlotIdx = (progressData.GetProgress(PLAYER_PROGRESS_PET_1_0) == ITEM_ID_BLANK)
                                        ? PLAYER_PROGRESS_PET_1_0
                                        : PLAYER_PROGRESS_PET_2_0;
                progressData.SetProgress((ePlayerProgress)freeSlotIdx, pTileExtra->basePet);
                progressData.SetProgress((ePlayerProgress)(freeSlotIdx + 1), pTileExtra->secondPet);
                progressData.SetProgress((ePlayerProgress)(freeSlotIdx + 2), pTileExtra->thirdPet);

                pTileExtra->basePet = ITEM_ID_BLANK;
                pTileExtra->secondPet = ITEM_ID_BLANK;
                pTileExtra->thirdPet = ITEM_ID_BLANK;

                pPlayer->SendOnTalkBubble("I stuffed " + pTileExtra->cageName + " the " +
                                              progressData.GetBattlePetName(freeSlotIdx) + " into my Battle Leash!",
                                          false);
                pWorld->SendConsoleMessageToAll(pPlayer->GetDisplayName(false) + "`o put " + pTileExtra->cageName +
                                                " the " + progressData.GetBattlePetName(freeSlotIdx) +
                                                " in their Battle Leash");
                pWorld->SendTileUpdate(pTile);

                pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_TRANSFORM_FX, pTile->GetWorldPosCenter());

                if (pPlayer->GetInventory().GetClothByPart(BODY_PART_HAND) == ITEM_ID_BATTLE_LEASH)
                {
                    pPlayer->ToggleBattlePetLeash(true);
                }

                return;
            }

            case "dropoff1"_hash:
            {
                if (pTileExtra->basePet != ITEM_ID_BLANK)
                    return;

                PlayerProgress& progressData = pPlayer->GetProgressData();
                if (progressData.GetProgress(PLAYER_PROGRESS_PET_1_0) == ITEM_ID_BLANK)
                    return;

                pTileExtra->basePet = progressData.GetProgress(PLAYER_PROGRESS_PET_1_0);
                pTileExtra->secondPet = progressData.GetProgress(PLAYER_PROGRESS_PET_1_1);
                pTileExtra->thirdPet = progressData.GetProgress(PLAYER_PROGRESS_PET_1_2);

                progressData.SetProgress(PLAYER_PROGRESS_PET_1_0, ITEM_ID_BLANK);
                progressData.SetProgress(PLAYER_PROGRESS_PET_1_1, ITEM_ID_BLANK);
                progressData.SetProgress(PLAYER_PROGRESS_PET_1_2, ITEM_ID_BLANK);

                if (pTileExtra->cageName.empty())
                {
                    pTileExtra->cageName = GetRandomGrowNamePart() + GetRandomGrowNamePart();
                }

                pWorld->SendTileUpdate(pTile);
                pPlayer->ToggleBattlePetLeash(true);

                pPlayer->SendOnTalkBubble(
                    "I caged " + pTileExtra->cageName + " the " + progressData.GetBattlePetName(0), false);
                pWorld->SendConsoleMessageToAll(pPlayer->GetDisplayName(false) + "`o caged " + pTileExtra->cageName +
                                                " the " + progressData.GetBattlePetName(0));

                pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_TRANSFORM_FX, pTile->GetWorldPosCenter());
                return;
            }

            case "dropoff2"_hash:
            {
                if (pTileExtra->basePet != ITEM_ID_BLANK)
                    return;

                PlayerProgress& progressData = pPlayer->GetProgressData();
                if (progressData.GetProgress(PLAYER_PROGRESS_PET_2_0) == ITEM_ID_BLANK)
                    return;

                pTileExtra->basePet = progressData.GetProgress(PLAYER_PROGRESS_PET_2_0);
                pTileExtra->secondPet = progressData.GetProgress(PLAYER_PROGRESS_PET_2_1);
                pTileExtra->thirdPet = progressData.GetProgress(PLAYER_PROGRESS_PET_2_2);

                progressData.SetProgress(PLAYER_PROGRESS_PET_2_0, ITEM_ID_BLANK);
                progressData.SetProgress(PLAYER_PROGRESS_PET_2_1, ITEM_ID_BLANK);
                progressData.SetProgress(PLAYER_PROGRESS_PET_2_2, ITEM_ID_BLANK);

                if (pTileExtra->cageName.empty())
                {
                    pTileExtra->cageName = GetRandomGrowNamePart() + GetRandomGrowNamePart();
                }

                pWorld->SendTileUpdate(pTile);
                if (pPlayer->GetActiveBattlePetSlot() == 1)
                {
                    pPlayer->ToggleBattlePetLeash(false);
                }

                pPlayer->SendOnTalkBubble(
                    "I caged " + pTileExtra->cageName + " the " + progressData.GetBattlePetName(1), false);
                pWorld->SendConsoleMessageToAll(pPlayer->GetDisplayName(false) + "`o caged " + pTileExtra->cageName +
                                                " the " + progressData.GetBattlePetName(1));

                pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_TRANSFORM_FX, pTile->GetWorldPosCenter());
                return;
            }

            default:
                return;
        }
    }

    pPlayer->SendOnDialogRequest(db.Get());
}

void BulletinBlockDialog::Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem)
{
    if (!pPlayer || !pTile || !pItem)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    TileExtra_Bulletin* pTileExtra = pTile->GetExtra<TileExtra_Bulletin>();
    if (!pTileExtra || pItem->type != ITEM_TYPE_BULLETIN)
        return;

    if (pTileExtra->letters.empty())
    {
        SendBulletinDialog(pPlayer, pWorld, pTile, pItem);
    }
    else
    {
        Vector2Int& vTilePos = pTile->GetPos();
        std::vector<int32> userIDs;
        userIDs.reserve(pTileExtra->letters.size());

        for (auto& letter : pTileExtra->letters)
        {
            userIDs.push_back(letter.userID);
        }

        GetUserCacheManager()->FetchMetadata(pPlayer->GetNetID(), CACHE_REQ_BULLETIN_BLOCK, userIDs,
                                             {pWorld->GetInstanceID(), vTilePos.x, vTilePos.y});
    }
}

void BulletinBlockDialog::HandleFromCache(GamePlayer* pPlayer, uint32 worldInstanceID, int32 tileX, int32 tileY)
{
    if (!pPlayer || pPlayer->GetCurrentWorld() != worldInstanceID)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(worldInstanceID);
    if (!pWorld)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if (!pTile)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem || pItem->type != ITEM_TYPE_BULLETIN)
        return;

    TileExtra_Bulletin* pTileExtra = pTile->GetExtra<TileExtra_Bulletin>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The board is gone!.", false);
        return;
    }

    SendBulletinDialog(pPlayer, pWorld, pTile, pItem);
}

void BulletinBlockDialog::RequestDeleteEntry(GamePlayer* pPlayer, TileInfo* pTile, int32 index)
{
    if (!pTile)
        return;

    TileExtra_Bulletin* pTileExtra = pTile->GetExtra<TileExtra_Bulletin>();
    if (!pTileExtra)
        return;

    if (index < 0 || pTileExtra->letters.size() <= index)
    {
        pPlayer->SendOnTalkBubble("Can't remove that, it's not there anymore!", false);
        return;
    }

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon("Delete ``\"" + pTileExtra->letters[index].message + "\"`` from your board?", pTile->GetFG(),
                          false)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y)
        .EmbedData("delete_index", index)
        .EndDialog("remove_bulletin", "OK", "Cancel");

    pPlayer->SendOnDialogRequest(db.Get());
}

void BulletinBlockDialog::SendBulletinDialog(GamePlayer* pPlayer, World* pWorld, TileInfo* pTile, ItemInfo* pItem)
{
    if (!pPlayer || !pWorld || !pTile || !pItem)
        return;

    if (pPlayer->GetCurrentWorld() != pWorld->GetInstanceID())
        return;

    TileExtra_Bulletin* pTileExtra = pTile->GetExtra<TileExtra_Bulletin>();
    if (!pTileExtra || pItem->type != ITEM_TYPE_BULLETIN)
        return;

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon(pItem->name, pItem->id, true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y)
        .AddSpacer();

    bool hideNames = pTileExtra->HasFlag(TILE_EXTRA_BULLETIN_HIDE_NAMES);
    bool hasAccessToEdit = pWorld->PlayerHasAccessOnTile(pPlayer, pTile);

    if (pTileExtra->letters.empty())
    {
        db.AddTextBox(pItem->name + " is empty.");
    }
    else
    {
        UserCacheManager* pUserMgr = GetUserCacheManager();
        for (uint32 i = 0; i < pTileExtra->letters.size(); ++i)
        {
            UserMetadata* pMetaData = pUserMgr->GetMetadata(pTileExtra->letters[i].userID);
            string shownMsg;

            if (!hideNames)
            {
                shownMsg += pMetaData ? pMetaData->displayName + ":`2 "
                                      : ("#" + ToString(pTileExtra->letters[i].userID) + ":`2 ");
            }
            shownMsg += pTileExtra->letters[i].message;
            db.AddLabelWithIconButton(ToString(i), shownMsg, ITEM_ID_LETTER);
        }
    }
    db.AddSpacer();

    bool canPostMessage = true;
    if (!hasAccessToEdit && !pTileExtra->HasFlag(TILE_EXTRA_BULLETIN_READ_ONLY))
    {
        uint32 letterCountFromUser = pTileExtra->GetCountOfLettersFromID(pPlayer->GetUserID());
        if (letterCountFromUser > 2)
        {
            db.AddTextBox("You already have `w " + ToString(letterCountFromUser) + "`` posts up, take a break!");
            canPostMessage = false;
        }
    }

    if (hasAccessToEdit)
    {
        db.AddLabelWithIcon("`wOwner Options", ITEM_ID_WORLD_LOCK, true);

        if (hideNames)
            db.AddTextBox("Uncheck `5Hide names`` to enable individual comment removal options.").AddSpacer();
        else
            db.AddTextBox("To remove an individual comment, press the icon to the left of it.").AddSpacer();

        if (canPostMessage)
        {
            db.AddTextBox("Add to conversation?")
                .AddTextInput("sign_text", "", "", 128)
                .AddSpacer()
                .AddButton("send", "`2Add");
        }

        if (!pTileExtra->letters.empty())
        {
            db.AddSpacer().AddButton("clear", "`4Clear Board");
        }

        db.AddCheckBox("checkbox_locked", "Public can add", !pTileExtra->HasFlag(TILE_EXTRA_BULLETIN_READ_ONLY));
        db.AddCheckBox("checkbox_hide", "Hide names", pTileExtra->HasFlag(TILE_EXTRA_BULLETIN_HIDE_NAMES));
    }
    else
    {
        if (canPostMessage && !pTileExtra->HasFlag(TILE_EXTRA_BULLETIN_READ_ONLY))
        {
            db.AddTextBox("Add to conversation?")
                .AddTextInput("sign_text", "", "", 128)
                .AddSpacer()
                .AddButton("send", "`2Add");
        }
    }

    if (hasAccessToEdit)
    {
        db.EndDialog("bulletin_edit", "OK", "Cancel");
    }
    else
    {
        if (!canPostMessage)
            db.EndDialog("bulletin_edit", "", "Cancel");
        else
            db.EndDialog("bulletin_edit", "", "Continue");
    }

    pPlayer->SendOnDialogRequest(db.Get());
}

void BulletinBlockDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
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

    TileExtra_Bulletin* pTileExtra = pTile->GetExtra<TileExtra_Bulletin>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The board is gone!", false);
        return;
    }

    if (pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
    {
        if (auto pLocked = packet.Find("checkbox_locked"_hash))
        {
            bool val;
            if (pLocked->GetBool(val) != TO_INT_SUCCESS)
                return;

            val ? pTileExtra->RemoveFlag(TILE_EXTRA_BULLETIN_READ_ONLY)
                : pTileExtra->SetFlag(TILE_EXTRA_BULLETIN_READ_ONLY);
        }

        if (auto pHide = packet.Find("checkbox_hide"_hash))
        {
            bool val;
            if (pHide->GetBool(val) != TO_INT_SUCCESS)
                return;

            val ? pTileExtra->SetFlag(TILE_EXTRA_BULLETIN_HIDE_NAMES)
                : pTileExtra->RemoveFlag(TILE_EXTRA_BULLETIN_HIDE_NAMES);
        }
    }
    else if (pTileExtra->HasFlag(TILE_EXTRA_BULLETIN_READ_ONLY))
        return;

    if (auto pButtonClicked = packet.Find("buttonClicked"_hash))
    {
        if (pButtonClicked->valueSize == 0 || pButtonClicked->valueSize > 8)
            return;

        std::string_view buttonClicked = pButtonClicked->GetStringView();

        if (buttonClicked == "send")
        {
            auto pSignText = packet.Find("sign_text"_hash);
            if (!pSignText)
                return;

            if (pSignText->valueSize > 128)
            {
                pPlayer->SendOnTalkBubble("That letter is too long!", false);
                return;
            }

            string text = pSignText->GetString();
            RemoveExtraWhiteSpaces(text);
            RemoveGTColorCodes(text);

            if (text.empty() || text.size() > 128)
                return;

            if (text.size() < 3)
            {
                pPlayer->SendOnTalkBubble("That's not interesting enough to mail.", false);
                return;
            }

            if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile) &&
                pTileExtra->GetCountOfLettersFromID(pPlayer->GetUserID()) > 2)
            {
                pPlayer->SendOnTalkBubble("Don't flood the board.", false);
                return;
            }

            uint32 totalStrLen = text.size();
            for (auto& letter : pTileExtra->letters)
            {
                totalStrLen += letter.message.size();
            }

            if (totalStrLen > 1024)
            {
                LOGGER_LOG_ERROR("Failed to write into bulletin totalStrLen (with text): %d, text size: %d, userID: %d",
                                 totalStrLen, text.size(), pPlayer->GetUserID());
                return;
            }

            pTileExtra->letters.push_back({pPlayer->GetUserID(), text});

            pPlayer->SendOnTalkBubble("`2Bulletin posted.``", false);
            pPlayer->PlaySFX("page_turn.wav");
            return;
        }

        if (buttonClicked == "clear")
        {
            if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
            {
                pPlayer->SendOnTalkBubble("It's not yours don't do that.", false);
                return;
            }

            pTileExtra->letters.clear();

            pPlayer->SendOnTalkBubble("`2Text cleared.``", false);
            pPlayer->PlaySFX("page_turn.wav");
            return;
        }

        if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
            return;

        int32 index = -1;
        if (pButtonClicked->GetInt(index) != TO_INT_SUCCESS || index < 0)
            return;

        RequestDeleteEntry(pPlayer, pTile, index);
    }
}

void BulletinBlockDialog::HandleDeleteEntry(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pTileX = packet.Find("tilex"_hash);
    if (!pTileX)
        return;

    auto pTileY = packet.Find("tiley"_hash);
    if (!pTileY)
        return;

    auto pDeleteIndex = packet.Find("delete_index"_hash);
    if (!pDeleteIndex)
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

    int32 deleteIndex = 0;
    if (pDeleteIndex->GetInt(deleteIndex) != TO_INT_SUCCESS)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if (!pTile)
        return;

    TileExtra_Bulletin* pTileExtra = pTile->GetExtra<TileExtra_Bulletin>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The board is gone!", false);
        return;
    }

    if (deleteIndex < 0 || pTileExtra->letters.size() <= deleteIndex)
    {
        pPlayer->SendOnTalkBubble("Can't remove that, it's not there anymore!", false);
        return;
    }

    if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem || pItem->type != ITEM_TYPE_BULLETIN)
        return;

    pTileExtra->letters.erase(pTileExtra->letters.begin() + deleteIndex);
    pPlayer->SendOnTalkBubble("`2Bulletin removed.``", false);
    pPlayer->PlaySFX("page_turn.wav");

    if (pItem->id == ITEM_ID_BULLETIN_BOARD)
    {
        BulletinBlockDialog::Request(pPlayer, pTile, pItem);
    }
}

void BurglarDialog::RequestPunch(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile)
        return;
}

void BurglarDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet) {}

void CrystalBlockDialog::Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem)
{
    if (!pPlayer || !pTile || !pItem)
        return;

    if (pItem->type != ITEM_TYPE_CRYSTAL)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    TileExtra_Crystal* pTileExtra = pTile->GetExtra<TileExtra_Crystal>();
    if (!pTileExtra)
        return;

    DialogBuilder db;
    db.SetDefaultColor('o').AddLabelWithIcon("`wCrystal Cluster", pItem->id, true);

    string clusterCrystals = "This is a cluster of ";

    for (uint32 i = 0; i < pTileExtra->crystals.size();)
    {
        char c = pTileExtra->crystals[i];
        int32 cnt = 0;

        while (i < pTileExtra->crystals.size() && pTileExtra->crystals[i] == c)
            cnt++, i++;

        clusterCrystals += (cnt == 1   ? "a few "
                            : cnt == 2 ? "several "
                            : cnt == 3 ? "lots of "
                            : cnt == 4 ? "tons of "
                                       : "nothing but ");

        clusterCrystals += (c == '1' ? "red" : c == '2' ? "green" : c == '3' ? "blue" : c == '4' ? "white" : "black");

        if (i < pTileExtra->crystals.size())
            clusterCrystals += ", ";
    }

    db.AddTextBox(clusterCrystals + "crystals.");

    if (pWorld->PlayerHasAccessOnTile(pPlayer, pTile) && pTileExtra->crystals.size() >= 5)
    {
        int16 totalChi[4] = {0};
        pWorld->CalcHarmonicCrystal(pTile, totalChi);

        int32 accur = gHarmonicCrystal.GetChiAccuracy(pTile, pWorld);
        bool ready = true;
        int16 diff[4] = {0};

        for (int32 i = 0; i < 4; ++i)
        {
            diff[i] = pTileExtra->chi[i] - totalChi[i];

            if (diff[i] < -accur || diff[i] > accur)
                ready = false;
        }

        string elements[4] = {"Earth", "Fire", "Air", "Water"};
        string elementInfo;

        if (!ready)
        {
            elementInfo = "The crystals are out of alignment with the elements. They need to be surrounded by ";
            bool first = true;

            for (int32 i = 0; i < 4; ++i)
            {
                string state;

                if (diff[i] < -accur)
                {
                    if (diff[i] < -accur * 5)
                        state = "vastly less";
                    else if (diff[i] < -accur * 3)
                        state = "far less";
                    else if (diff[i] < -accur * 2)
                        state = "less";
                    else
                        state = "slightly less";
                }
                else if (diff[i] > accur)
                {
                    if (diff[i] > accur * 5)
                        state = "vastly more";
                    else if (diff[i] > accur * 3)
                        state = "far more";
                    else if (diff[i] > accur * 2)
                        state = "more";
                    else
                        state = "slightly more";
                }
                else
                    continue;

                if (!first)
                    elementInfo += ", ";

                first = false;
                elementInfo += state + " " + elements[i];
            }

            elementInfo += " to seek a higher form.";
        }
        else
        {
            elementInfo = "The crystals are in perfect harmonic resonance with the elements. A single tap should "
                          "reveal their true essence.";
        }

        db.AddTextBox(elementInfo);
    }

    db.EndDialog("crystal_edit", "", "Cool, man");
    pPlayer->SendOnDialogRequest(db.Get());
}

void DisplayBlockDialog::Request(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    TileExtra_DisplayBlock* pTileExtra = pTile->GetExtra<TileExtra_DisplayBlock>();
    if (!pTileExtra)
        return;

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon("`wDisplay Block``", pTile->GetFG(), true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y);

    if (pTileExtra->itemID == ITEM_ID_BLANK)
    {
        db.AddTextBox("The Display Block is empty. Use an item on it to display the item!");
    }
    else
    {
        ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTileExtra->itemID);
        if (!pItem)
            return;

        db.AddTextBox("A " + pItem->name + " in on display here.");
    }

    if (pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
        db.EndDialog("displayblock", "Pick it up", "Leave it");
    else
        db.EndDialog("displayblock", "", "Okay");

    pPlayer->SendOnDialogRequest(db.Get());
}

void DisplayBlockDialog::RequestPutItem(GamePlayer* pPlayer, TileInfo* pTile, int32 itemID)
{
    if (!pPlayer || !pTile)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
        return;

    TileExtra_DisplayBlock* pTileExtra = pTile->GetExtra<TileExtra_DisplayBlock>();
    if (!pTileExtra)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    if (!pWorld->GetTileManager()->IsTileLockedWithLock(pTile))
    {
        pPlayer->SendOnTalkBubble("This area must be locked to put your item on display!", false);
        return;
    }

    if (!pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
    {
        pPlayer->SendOnTalkBubble("Only the block's owner can place items in it.", false);
        return;
    }

    if (pPlayer->GetInventory().GetCountOfItem(pItem->id) < 1)
    {
        pPlayer->SendOnTalkBubble("Your item vanished!", false);
        return;
    }

    if (pTileExtra->itemID != ITEM_ID_BLANK)
    {
        pPlayer->SendOnTalkBubble("Remove what's in there first!", false);
        return;
    }

    if (pItem->type == ITEM_TYPE_DISPLAY_BLOCK || pItem->type == ITEM_TYPE_LOCK)
    {
        pPlayer->SendOnTalkBubble("Sorry, no displaying Display Blocks or Locks.", false);
        return;
    }

    if (pItem->id == ITEM_ID_SCREEN_DOOR)
    {
        pPlayer->SendOnTalkBubble("Don't be a scammer", false);
        return;
    }

    if (pItem->HasFlag(ITEM_FLAG_UNTRADEABLE))
    {
        pPlayer->SendOnTalkBubble("You can't display untradeable items.", false);
        return;
    }

    if (pItem->type == ITEM_TYPE_PETFISH)
    {
        pPlayer->SendOnTalkBubble("If you wanna display a fish, use a fish tank!", false);
        return;
    }

    if (pItem->id == ITEM_ID_WORLD_KEY || pItem->id == ITEM_ID_GUILD_KEY || pItem->id == ITEM_ID_MAGPLANT_5000_REMOTE)
    {
        pPlayer->SendOnTalkBubble("No sir.", false);
        return;
    }

    pTileExtra->itemID = pItem->id;
    pPlayer->ModifyInventoryItem(pItem->id, -1);

    pWorld->SendPlayPositionedToAll(pPlayer, "blorb.wab");
    pWorld->ThrowItemToPositionFromPlayer(pPlayer, pTile->GetWorldPosCenter(), pItem->id, 1);
    pWorld->SendTileUpdate(pTile);
}

void DisplayBlockDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
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

    TileExtra_DisplayBlock* pTileExtra = pTile->GetExtra<TileExtra_DisplayBlock>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The display is gone!", false);
        return;
    }

    if (!pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
        return;

    if (pTileExtra->itemID == ITEM_ID_BLANK)
    {
        pPlayer->SendOnTalkBubble("There's nothing in there!", false);
        return;
    }

    if (!pPlayer->GetInventory().HaveRoomForItem(pTileExtra->itemID, 1))
    {
        pPlayer->SendOnTalkBubble("You don't have room to pick that up!", false);
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTileExtra->itemID);

    pPlayer->ModifyInventoryItem(pTileExtra->itemID, 1);
    pTileExtra->itemID = ITEM_ID_BLANK;

    pWorld->SendTileUpdate(pTile);

    if (pItem)
    {
        pPlayer->SendOnTalkBubble("You removed `5" + pItem->name, false);
    }
}

void DonationBoxDialog::Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem)
{
    if (!pPlayer || !pTile || !pItem)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    TileExtra_DonaitonBox* pTileExtra = pTile->GetExtra<TileExtra_DonaitonBox>();
    if (!pTileExtra)
        return;

    if (pPlayer->GetDistToTileInTiles(pTile) > 4)
    {
        pPlayer->SendOnTalkBubble("`5[`2I'll need to get closer``]``", false);
        return;
    }

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon(pItem->name, pItem->id, true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y);

    if (pTileExtra->gifts.empty())
    {
        db.AddTextBox("The box is currently empty.");
    }

    if (pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
    {
        if (!pTileExtra->gifts.empty())
        {
            std::vector<int32> userIDs;
            userIDs.reserve(pTileExtra->gifts.size());

            for (auto& gift : pTileExtra->gifts)
            {
                userIDs.push_back(gift.userID);
            }

            GetUserCacheManager()->FetchMetadata(pPlayer->GetNetID(), CACHE_REQ_DONATION_BOX_BLOCK, userIDs,
                                                 {pWorld->GetInstanceID(), vTilePos.x, vTilePos.y});
            return;
        }

        db.AddSpacer().AddTextInput("min_rarity", "Min Rarity:", ToString(pTileExtra->minRarity), 3);

        bool canGift = true;
        if (pTileExtra->gifts.size() >= 20)
        {
            db.AddTextBox("This box already has `w" + ToString(pTileExtra->gifts.size()) +
                          "`` gifts in it, can't add mroe until you cleam them.");
            canGift = false;
        }

        if (canGift)
        {
            db.AddSpacer().AddItemPicker("itemid",
                                         "`wGive Gift`` (Min Rarity:`5 " + ToString(pTileExtra->minRarity) + "``)",
                                         "Choose an item to give");
        }

        db.EndDialog("donation_box_edit", "Update", "Cancel");
        pPlayer->SendOnDialogRequest(db.Get());
    }
    else
    {
        if (!pTileExtra->gifts.empty())
        {
            db.AddTextBox("You see `w" + ToString(pTileExtra->gifts.size()) + "`` gifts in the box!");
        }

        bool canGift = true;
        if (pTileExtra->gifts.size() >= 20)
        {
            db.AddTextBox("This box already has `w" + ToString(pTileExtra->gifts.size()) +
                          "`` gifts in it. Try again later.");

            canGift = false;
        }

        uint32 playerGiftCount = pTileExtra->GetCountOfGiftsFromID(pPlayer->GetUserID());
        if (canGift && playerGiftCount > 2)
        {
            db.AddTextBox("You've already crammed `w" + ToString(playerGiftCount) +
                          "`` of your gifts into the box, better wait.");

            canGift = false;
        }

        if (canGift)
        {
            db.AddTextBox("Want to leave a gift for the owner?")
                .AddItemPicker("itemid", "`wGive Gift`` (Min Rarity:`5" + ToString(pTileExtra->minRarity) + "``)",
                               "Choose an item to give");
        }
    }

    db.EndDialog("donation_box_edit", "", "Cancel");
    pPlayer->SendOnDialogRequest(db.Get());
}

void DonationBoxDialog::HandleFromCache(GamePlayer* pPlayer, uint32 worldInstanceID, int32 tileX, int32 tileY)
{
    if (!pPlayer || pPlayer->GetCurrentWorld() != worldInstanceID)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(worldInstanceID);
    if (!pWorld)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if (!pTile)
        return;

    TileExtra_DonaitonBox* pTileExtra = pTile->GetExtra<TileExtra_DonaitonBox>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The box is gone!.", false);
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (pItem->type != ITEM_TYPE_DONATION_BOX)
        return;

    if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        return;

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon(pItem->name, pItem->id, true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y)
        .AddTextBox("You have `w" + ToString(pTileExtra->gifts.size()) + "`` gift" +
                    (pTileExtra->gifts.size() > 1 ? "s:" : ":") + "waiting:")
        .AddSpacer();

    UserCacheManager* pUserMgr = GetUserCacheManager();
    ItemInfoManager* pItemMgr = GetItemInfoManager();

    for (auto& gift : pTileExtra->gifts)
    {
        ItemInfo* pItem = pItemMgr->GetItemByID(gift.itemID);
        if (!pItem)
            continue;

        UserMetadata* pMetaData = pUserMgr->GetMetadata(gift.userID);
        string donationMsg;
        donationMsg.reserve(128);

        donationMsg = pItem->name + " (`w" + ToString(gift.itemCount) + "``) from `w" +
                      (pMetaData ? pMetaData->displayName : ("#" + ToString(gift.userID)));

        if (!gift.message.empty())
        {
            donationMsg += "`5 - \" " + gift.message + " \"";
        }

        db.AddCheckBox("checkbox", donationMsg, false);
    }

    db.AddSpacer()
        .AddButton("clear_selected", "`4Retrieve Selected Gifts``")
        .AddButton("clear", "`4Retrieve All Gifts``")
        .AddTextInput("min_rarity", "Min Rarity:", ToString(pTileExtra->minRarity), 3);

    bool canGift = true;
    if (pTileExtra->gifts.size() >= 20)
    {
        db.AddTextBox("This box already has `w" + ToString(pTileExtra->gifts.size()) +
                      "`` gifts in it, can't add mroe until you cleam them.");
        canGift = false;
    }

    if (canGift)
    {
        db.AddSpacer().AddItemPicker("itemid",
                                     "`wGive Gift`` (Min Rarity:`5 " + ToString(pTileExtra->minRarity) + "``)",
                                     "Choose an item to give");
    }

    db.EndDialog("donation_box_edit", "Update", "Cancel");
    pPlayer->SendOnDialogRequest(db.Get());
}

void DonationBoxDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
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

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem || pItem->type != ITEM_TYPE_DONATION_BOX)
        return;

    TileExtra_DonaitonBox* pTileExtra = pTile->GetExtra<TileExtra_DonaitonBox>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The box is gone!", false);
        return;
    }

    if (auto pButtonClicked = packet.Find("buttonClicked"_hash))
    {
        if (pButtonClicked->valueSize == 0)
            return;

        if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        {
            pPlayer->SendOnTalkBubble("You can't figure out how to open it.", false);
            return;
        }

        std::string_view buttonClicked = pButtonClicked->GetStringView();
        if (buttonClicked == "clear_selected")
        {
            std::vector<bool> selectedGifts;
            const std::string_view accPrefix = "checkbox";

            bool found = false;

            for (uint8 i = 0; i < packet.count; ++i)
            {
                if (packet.fields[i].keySize == 0)
                    continue;

                std::string_view key = packet.fields[i].GetKeyStringView();
                if (key.size() != accPrefix.size() || key.substr(0, accPrefix.size()) != accPrefix)
                    continue;

                found = true;

                bool isChecked;
                if (packet.fields[i].GetBool(isChecked) != TO_INT_SUCCESS)
                {
                    selectedGifts.push_back(false);
                    continue;
                }

                selectedGifts.push_back(isChecked);
            }

            if (!found)
                return;

            EmptyDonationBox(pPlayer, pTile, false, selectedGifts);
        }
        else if (buttonClicked == "clear")
        {
            EmptyDonationBox(pPlayer, pTile, false);
        }
        else
            return;

        if (!pTileExtra->gifts.empty())
        {
            pPlayer->SendOnTalkBubble("`2(Couldn't get all of the gifts)``", false);
        }
        else
        {
            pPlayer->SendOnTalkBubble("`2Box emptied.``", false);
            pPlayer->PlaySFX("page_turn.wav");
        }
    }
    else
    {
        if (auto pItemID = packet.Find("itemid"_hash))
        {
            uint32 itemID = 0;
            if (pItemID->GetUInt(itemID) != TO_INT_SUCCESS)
                return;

            RequestDonatingItem(pPlayer, pTile, itemID);
            return;
        }
    }

    if (auto pMinRarity = packet.Find("min_rarity"_hash))
    {
        int32 minRarity = 0;
        if (pMinRarity->GetInt(minRarity) != TO_INT_SUCCESS)
            return;

        if (minRarity == pTileExtra->minRarity)
            return;

        if (minRarity <= 0)
        {
            pPlayer->SendOnTalkBubble("That not gonna work.", false);
            return;
        }

        if (minRarity > 999)
        {
            pPlayer->SendOnTalkBubble("Min rarity must be from 1-999", false);
            return;
        }

        pTileExtra->minRarity = minRarity;
        pPlayer->SendOnTalkBubble("Updated minimum acceptable item rarity to `2" + ToString(minRarity) + "``!", false);
    }
    else
    {
        pTileExtra->minRarity = 2;
    }
}

void DonationBoxDialog::EmptyDonationBox(GamePlayer* pPlayer, TileInfo* pTile, bool allowDrop,
                                         const std::vector<bool>& selectedGifts)
{
    if (!pPlayer || !pTile)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    TileExtra_DonaitonBox* pTileExtra = pTile->GetExtra<TileExtra_DonaitonBox>();
    if (!pTileExtra)
        return;

    if (pTileExtra->gifts.empty())
    {
        pPlayer->SendOnTalkBubble("`4The box is already empty", false);
        return;
    }

    ItemInfoManager* pItemMgr = GetItemInfoManager();
    UserCacheManager* pUserCacheMgr = GetUserCacheManager();

    int32 index = 0;
    bool isBackpackFull = false;

    auto emptyIfCan = std::remove_if(
        pTileExtra->gifts.begin(), pTileExtra->gifts.end(),
        [&](const TileDonationBoxGifts& gift)
        {
            ItemInfo* pItem = pItemMgr->GetItemByID(gift.itemID);
            if (!pItem)
            {
                index++;
                return true;
            }

            bool takeItem = true;

            if (!selectedGifts.empty())
            {
                if (index >= selectedGifts.size() || !selectedGifts[index])
                {
                    takeItem = false;
                }
            }

            if (takeItem)
            {
                if (pPlayer->GetInventory().GetCountOfItem(pItem->id) + gift.itemCount > pItem->maxCanHold)
                {
                    takeItem = false;
                    isBackpackFull = true;
                }
            }

            index++;

            if (takeItem)
            {
                pPlayer->ModifyInventoryItem(pItem->id, gift.itemCount);

                string consoleMsg;
                consoleMsg.reserve(128);

                auto pUser = pUserCacheMgr->GetMetadata(gift.userID);

                if (pUser)
                {
                    consoleMsg = "[`o" + pPlayer->GetDisplayName(false) + "`w receives `5" + ToString(gift.itemCount) +
                                 "`w " + pItem->name + " from `w" + pUser->displayName + "`o, how nice!]";
                }
                else
                {
                    consoleMsg = "[`o" + pPlayer->GetDisplayName(false) + "`w receives `5" + ToString(gift.itemCount) +
                                 "`w " + pItem->name + "`o, how nice!]";
                }

                pWorld->SendConsoleMessageToAll(consoleMsg);
                return true;
            }
            else if (allowDrop)
            {
                pWorld->DropObjectOnTile(pTile, pItem->id, gift.itemCount, GetRandomItemDropOffset(), true);
                return true;
            }

            return false;
        });

    bool hasChanged = (emptyIfCan != pTileExtra->gifts.end());
    if (hasChanged)
    {
        pTileExtra->gifts.erase(emptyIfCan, pTileExtra->gifts.end());
    }

    if (isBackpackFull)
    {
        pPlayer->SendOnTalkBubble("I dont have enough room in my backpack to get item(s) from the box!", false);
    }

    if (pTileExtra->gifts.empty())
    {
        pTile->RemoveFlag(TILE_FLAG_IS_ON);
        pWorld->SendTileUpdate(pTile);
    }
}

void DonationBoxDialog::RequestDonatingItem(GamePlayer* pPlayer, TileInfo* pTile, int32 itemID)
{
    if (!pPlayer || !pTile)
        return;

    if (pPlayer->GetCurrentWorld() == 0)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
        return;

    InventoryItemInfo* pInvItem = pPlayer->GetInventory().GetItemByID(pItem->id);
    if (!pInvItem)
        return;

    TileExtra_DonaitonBox* pTileExtra = pTile->GetExtra<TileExtra_DonaitonBox>();
    if (!pTileExtra)
        return;

    if (pTileExtra->lastDonationTime.GetElapsedTime() < 5000)
    {
        pPlayer->SendOnTalkBubble("`7[```4The donation box needs to warm up for 5 seconds before use!`7]``", false);
        return;
    }

    if (pItem->rarity != 0 && pItem->rarity < pTileExtra->minRarity && !pItem->HasFlag(ITEM_FLAG_SEEDLESS))
    {
        pPlayer->SendOnTalkBubble("`7[```4This box only accepts items rarity " + ToString(pTileExtra->minRarity) +
                                      "+ or greater`7]``",
                                  false);
        return;
    }

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon(pItem->name, pItem->id, true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y)
        .EmbedData("itemID", ToItemClientID(pItem->id))
        .AddTextBox("How many to put in the box as a gift? (Note: You will `4LOSE`` the items you give!)")
        .AddTextInput("count", "Count:", "", 5)
        .AddTextInput("sign_text", "Optional Note:", "", 128)
        .AddSpacer()
        .AddButton("give", "`4Give the item(s)``")
        .AddSpacer()
        .AddButton("cancel", "`wCancel``")
        .EndDialog("give_item", "", "");

    pPlayer->SendOnDialogRequest(db.Get());
}

void DonationBoxDialog::HandleGiveItem(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pTileX = packet.Find("tilex"_hash);
    if (!pTileX)
        return;

    auto pTileY = packet.Find("tiley"_hash);
    if (!pTileY)
        return;

    auto pButtonClicked = packet.Find("buttonClicked"_hash);
    if (!pButtonClicked || pButtonClicked->valueSize < 4)
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

    ItemInfo* pTileItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pTileItem || pTileItem->type != ITEM_TYPE_DONATION_BOX)
        return;

    TileExtra_DonaitonBox* pTileExtra = pTile->GetExtra<TileExtra_DonaitonBox>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The box is gone!", false);
        return;
    }

    std::string_view buttonClicked = pButtonClicked->GetStringView();
    if (buttonClicked == "cancel")
    {
        Request(pPlayer, pTile, pTileItem);
    }
    else if (buttonClicked == "give")
    {
        auto pItemID = packet.Find("itemID"_hash);
        auto pCount = packet.Find("count"_hash);
        if (!pItemID || !pCount)
            return;

        auto pSignText = packet.Find("sign_text"_hash);
        if (pSignText && pSignText->valueSize > 256)
            return;

        uint32 itemID = 0;
        if (pItemID->GetUInt(itemID) != TO_INT_SUCCESS)
            return;

        ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
        if (!pItem)
            return;

        int32 count = 0;
        if (pCount->GetInt(count) != TO_INT_SUCCESS)
            return;

        if (count == 0)
            return;

        if (count < 0)
        {
            pPlayer->SendOnTalkBubble("That would only have worked if someone programmed this function!", false);
            return;
        }

        if (pTileExtra->gifts.size() >= 20)
        {
            pPlayer->SendOnTalkBubble("You aren't able to fit another gift inside, it's jammed full.", false);
            return;
        }

        string signText = pSignText ? pSignText->GetString() : "";
        RemoveGTColorCodes(signText);
        StripWhiteSpace(signText);

        if (signText.size() > 128)
            return;

        if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        {
            int32 giftCount = pTileExtra->GetCountOfGiftsFromID(pPlayer->GetUserID());
            if (giftCount > 2)
            {
                pPlayer->SendOnTalkBubble("`4Woah nelly, you don't want to be TOO nice.  Try again later.``", false);
                return;
            }
        }

        InventoryItemInfo* pInvItem = pPlayer->GetInventory().GetItemByID(pItem->id);
        if (!pInvItem || pInvItem->count < count || count > pItem->maxCanHold)
        {
            pPlayer->SendOnTalkBubble("You don't have that to give!", false);
            return;
        }

        if (pItem->type == ITEM_TYPE_PETFISH)
        {
            pPlayer->SendOnTalkBubble("You can't put a live fish in the box!  That would just be mean.", false);
            return;
        }

        if (pItem->rarity != 0 && pItem->rarity < pTileExtra->minRarity && !pItem->HasFlag(ITEM_FLAG_SEEDLESS))
        {
            pPlayer->SendOnTalkBubble("`7[```4This box only accepts items rarity " + ToString(pTileExtra->minRarity) +
                                          "+ or greater`7]``",
                                      false);
            return;
        }

        uint32 totalStrLen = signText.size();
        for (auto& gift : pTileExtra->gifts)
        {
            totalStrLen += gift.message.size();
        }

        if (totalStrLen > 2048)
        {
            LOGGER_LOG_ERROR("Failed to write into donate box totalStrLen (with text): %d, text size: %d, userID: %d",
                             totalStrLen, signText.size(), pPlayer->GetUserID());
            return;
        }

        pPlayer->ModifyInventoryItem(pItem->id, -count);
        pTileExtra->gifts.push_back({pPlayer->GetUserID(), signText, (int16)pItem->id, (int16)count});

        pWorld->SendTalkBubbleAndConsoleToAll("`7[`w" + pPlayer->GetDisplayName(false) + "`w places `5" +
                                                  ToString(count) + "`2 " + pItem->name + "`w into the `w" +
                                                  pTileItem->name + "`7]``!",
                                              false, pPlayer);
        pPlayer->PlaySFX("page_turn.wav");

        if (pTileExtra->gifts.size() == 1)
        {
            pTile->SetFlag(TILE_FLAG_IS_ON);
            pWorld->SendTileUpdate(pTile);
        }
    }
}

void DoorDialog::Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem)
{
    if (!pPlayer || !pTile || !pItem)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    if (!pTile->IsTileExtraType(TILE_EXTRA_TYPE_DOOR))
        return;

    if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        return;

    TileExtra_Door* pTileExtra = pTile->GetExtra<TileExtra_Door>();
    if (!pTileExtra)
        return;

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon("`wEdit " + pItem->name, pItem->id, true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y)
        .AddTextInput("door_name", "Label", pTileExtra->name, 100)
        .AddTextInput("door_target", "Destination", pTileExtra->text, 24)
        .AddSmallText("Enter a Destination in this format: `2WORLDNAME:ID``")
        .AddSmallText("Leave `2WORLDNAME`` blank (:ID) to go to the door with `2ID`` in the `2Current World``.");

    if (pTile->GetFG() == ITEM_ID_PASSWORD_DOOR || pTile->GetFG() == ITEM_ID_HAUNTED_DOOR)
        db.AddTextInput("door_id", "Password", pTileExtra->id, 23);
    else
    {
        db.AddTextInput("door_id", "ID", pTileExtra->id, 11)
            .AddSmallText("Set a unique `2ID`` to target this door as a Destination from another!");
    }

    if (pWorld->GetTileManager()->IsTileLockedWithLock(pTile))
    {
        db.AddCheckBox("checkbox_locked", "Is open to public", !pTileExtra->HasFlag(TILE_EXTRA_LOCKED));
    }

    db.EndDialog("door_edit", "OK", "Cancel");
    pPlayer->SendOnDialogRequest(db.Get());
}

void DoorDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
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

    if (IsMainDoor(pTile->GetFG()))
        return;

    TileExtra_Door* pTileExtra = pTile->GetExtra<TileExtra_Door>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The door is gone!", false);
        return;
    }

    auto pDoorName = packet.Find("door_name"_hash);
    auto pDoorTarget = packet.Find("door_target"_hash);
    auto pDoorID = packet.Find("door_id"_hash);

    if (pDoorTarget && pDoorTarget->valueSize > 24)
    {
        pPlayer->SendOnTalkBubble("That target world name is too long!", false);
        return;
    }

    if (pDoorName && pDoorName->valueSize > 100)
        return;

    if (pDoorName && pDoorName->GetStringView().find("__&%@PL@%&__") != string::npos)
    {
        pPlayer->SendOnTalkBubble("You try to write the magic symbols down but they disappear!", false);
        return;
    }

    uint32 idLimit = (pTile->GetFG() == ITEM_ID_PASSWORD_DOOR || pTile->GetFG() == ITEM_ID_HAUNTED_DOOR ? 24 : 12);
    if (pDoorID && pDoorID->valueSize > idLimit)
    {
        if (pTile->GetFG() == ITEM_ID_PASSWORD_DOOR || pTile->GetFG() == ITEM_ID_HAUNTED_DOOR)
            pPlayer->SendOnTalkBubble("That password is too long!", false);
        else
            pPlayer->SendOnTalkBubble("That door ID is too long!", false);
        return;
    }

    string doorTarget;
    if (pDoorTarget)
    {
        doorTarget = ToUpper(pDoorTarget->GetString());
        RemoveGTColorCodes(doorTarget);
    }

    if (!doorTarget.empty() && !IsValidWorldName(doorTarget, true))
    {
        pPlayer->SendOnTalkBubble("Sorry, spaces and special characters are not allowed in world or door names.",
                                  false);
        return;
    }

    string doorID;
    if (pDoorID)
    {
        doorID = ToUpper(pDoorID->GetString());
        RemoveGTColorCodes(doorID);
    }

    if (!doorID.empty() && !IsValidWorldName(doorID))
    {
        pPlayer->SendOnTalkBubble("Sorry, spaces and special characters are not allowed in world or door names.",
                                  false);
        return;
    }

    string doorName;
    if (pDoorName)
    {
        doorName = pDoorName->GetString();
    }

    if (pTile->GetFG() == ITEM_ID_PASSWORD_DOOR || pTile->GetFG() == ITEM_ID_HAUNTED_DOOR && doorName.empty())
    {
        doorName = "Password Door";
    }

    if (pWorld->GetTileManager()->IsTileLockedWithLock(pTile))
    {
        if (auto pIsLocked = packet.Find("checkbox_locked"_hash))
        {
            bool val;
            if (pIsLocked->GetBool(val) != TO_INT_SUCCESS)
                return;

            val ? pTileExtra->RemoveFlag(TILE_EXTRA_LOCKED) : pTileExtra->SetFlag(TILE_EXTRA_LOCKED);
        }
    }

    pTileExtra->name = doorName;
    pTileExtra->text = doorTarget;
    pTileExtra->id = doorID;

    pWorld->SendTileUpdate(pTile);
}

void DoorDialog::RequestPasswordDoor(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem)
{
    if (!pPlayer || !pTile || !pItem)
        return;

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon(pItem->name, pItem->id, true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y)
        .AddTextBox("The door requires a password.")
        .AddTextInput("password", "Password", "", 24)
        .EndDialog("password_reply", "OK", "Cancel");

    pPlayer->SendOnDialogRequest(db.Get());
}

void DoorDialog::HandlePasswordReply(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    if (!pPlayer)
        return;

    auto pTileX = packet.Find("tilex"_hash);
    if (!pTileX)
        return;

    auto pTileY = packet.Find("tiley"_hash);
    if (!pTileY)
        return;

    auto pPassword = packet.Find("password"_hash);
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

    if (pTile->GetFG() != ITEM_ID_PASSWORD_DOOR && pTile->GetFG() != ITEM_ID_HAUNTED_DOOR)
        return;

    if (pPlayer->GetDistToTileInTiles(pTile) > 2)
    {
        pPlayer->SendOnTalkBubble("The door can't hear me say the password from this distance.", false);
        return;
    }

    TileExtra_Door* pTileExtra = pTile->GetExtra<TileExtra_Door>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The door is gone!", false);
        return;
    }

    if (pPassword->valueSize != pTileExtra->id.size() ||
        ToLower(pPassword->GetString().data()) != ToLower(pTileExtra->id))
    {
        pPlayer->SendOnTalkBubble("`4Wrong password!``", false);
        return;
    }

    if (pTileExtra->text.empty())
    {
        pPlayer->SendOnTalkBubble("`2The door opens! But nothing is behind it.``", false);
        return;
    }

    pPlayer->SendOnTalkBubble("`2The door opens!``", false);
    pPlayer->SendOnSetFreezeState(PLAYER_FREEZE_STATE_FROZEN, 0);

    auto targetWorld = Split(pTileExtra->text, ':');
    if (targetWorld.empty())
    {
        targetWorld.push_back("");
    }

    if (targetWorld[0].empty())
    {
        targetWorld[0] = pWorld->GetWorlName();
    }

    string targetDoorID = (targetWorld.size() > 1) ? targetWorld[1] : "";
    pPlayer->SetTargetJoinWorld(targetWorld[0], targetDoorID);
}

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

void DropItemDialog::Request(GamePlayer* pPlayer, InventoryItemInfo* pInvItem)
{
    if (!pPlayer || !pInvItem)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pInvItem->id);
    if (!pItem)
        return;

    if (pPlayer->GetCurrentWorld() == 0)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon("`wDrop " + pItem->name + "``", pItem->id, true)
        .AddTextBox("How many to drop?")
        .AddTextInput("count", "", ToString(pInvItem->count), 5)
        .EmbedData("itemID", ToItemClientID(pItem->id));

    bool showWarning = false;

    TileInfo* pLockTile = pWorld->GetTileManager()->GetKeyTile(KEY_TILE_WORLD_LOCK);
    if (!pLockTile)
    {
        showWarning = true;
    }
    else
    {
        TileExtra_Lock* pLockExtra = pLockTile->GetExtra<TileExtra_Lock>();
        if (!pLockExtra && (pLockExtra && !pLockExtra->HasAccess(pPlayer->GetUserID())))
        {
            showWarning = true;
        }
    }

    if (showWarning)
    {
        switch (RandomRangeInt(0, 4))
        {
            case 0:
                db.AddTextBox("`4Warning:`` Once you drop an item, it is no longer yours...");
                break;
            case 1:
                db.AddTextBox("`4Warning:`` Dropped items cannot be restored...");
                break;
            case 2:
                db.AddTextBox("`4Warning:`` Scammers may ask you to drop items...");
                break;
            case 3:
                db.AddTextBox("`4Warning:`` Players asking you to drop items may be scamming you.");
                break;
            case 4:
                db.AddTextBox("`4Warning:`` If trading, use the trade system instead of dropping items!");
                break;
        }
    }

    db.EndDialog("drop_item", "OK", "Cancel");
    pPlayer->SendOnDialogRequest(db.Get());
}

void DropItemDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer || pPlayer->GetCurrentWorld() == 0)
        return;

    auto pItemID = packet.Find("itemID"_hash);
    auto pCount = packet.Find("count"_hash);

    if (!pItemID || !pCount)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    uint32 itemID = 0;
    if (pItemID->GetUInt(itemID) != TO_INT_SUCCESS)
        return;

    int32 count = 0;
    if (pCount->GetInt(count) != TO_INT_SUCCESS)
        return;

    if (count == 0)
        return;

    if (count < 0)
    {
        pPlayer->SendOnTalkBubble("Nice try. You remind me of myself at that age.", true);
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
        return;

    pPlayer->DropItem(pItem->id, count, false);
}

void GatewayDialog::Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem)
{
    if (!pPlayer || !pTile || !pItem)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    if (pItem->type != ITEM_TYPE_GATEWAY)
        return;

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon("`wEdit " + pItem->name + "``", pItem->id, true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y);

    if (pWorld->GetTileManager()->IsTileLockedWithLock(pTile))
        db.AddCheckBox("checkbox_public", "Is open to public", pTile->HasFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC));
    else
        db.AddTextBox("This object has additional properties to edit if in a locked area.");

    db.EndDialog("gateway_edit", "OK", "Cancel");

    pPlayer->SendOnDialogRequest(db.Get());
}

void GatewayDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

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

    if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        return;

    bool tileNeedsUpdate = false;

    if (auto pPublic = packet.Find("checkbox_public"_hash))
    {
        bool val;
        if (pPublic->GetBool(val) != TO_INT_SUCCESS)
            return;

        if (val != pTile->HasFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC))
        {
            tileNeedsUpdate = true;
        }

        val ? pTile->SetFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC) : pTile->RemoveFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC);
    }

    if (tileNeedsUpdate)
    {
        pWorld->SendTileUpdate(pTile);
    }
}

/**
 * i need help with dialog im so bad at it lol
 */

void ItemFinderDialog::Render(GamePlayer* pPlayer)
{
    if (!pPlayer)
        return;

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon("Item Finder", ITEM_ID_GROWSCAN_9000, true)
        .AddSmallText("`2" + ToString(GetTotalCount()) + "`` items found. Listing `w" + ToString(m_pageSize) +
                      "`` items per page (Page `w" + ToString(m_page + 1) + "`5/`w" + ToString(GetMaxPageCount()) +
                      "``)")
        .AddSpacer();

    uint32 start = m_page * m_pageSize;
    uint32 end = Min(start + m_pageSize, GetTotalCount());

    for (uint32 i = start; i < end; ++i)
    {
        db.AddButtonWithIcon(ToString(i - start), "", m_fileteredItemIds[i]);
    }

    db.AddSpacer(true);

    if (m_page > 0)
        db.AddButton("prev", "`5< Previous");
    if (end < GetTotalCount())
        db.AddButton("next", "`5Next >");

    db.EndDialog("item_finder", "", "Close");
    pPlayer->SendOnDialogRequest(db.Get(), -1, true);
}

void ItemFinderDialog::OnSelectElement(GamePlayer* pPlayer, uint32 absoluteIndex)
{
    if (!pPlayer)
        return;

    Role* pRole = pPlayer->GetRole();
    if (!pRole || !pRole->HasPerm("command.finditem"_hash))
        return;

    int32 itemID = m_fileteredItemIds[absoluteIndex];
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
    {
        pPlayer->SendOnConsoleMessage("`4HUH?!`` Looks like the item lost in the universe.");
        return;
    }

    int32 itemCount = Max(0, pItem->maxCanHold - pPlayer->GetInventory().GetCountOfItem(pItem->id));
    if (itemCount == 0 || !pPlayer->GetInventory().HaveRoomForItem(itemID, itemCount))
    {
        pPlayer->SendOnConsoleMessage("`4Error``: You don't have room for that item!");
        return;
    }

    pPlayer->ModifyInventoryItem(itemID, itemCount);
    pPlayer->SendOnConsoleMessage("`oGiven: " + ToString(itemCount) + " " + pItem->name);
    pPlayer->ClosePaginatedDialog();
}

void LockDialog::Request(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile || pPlayer->GetCurrentWorld() == 0)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    TileExtra_Lock* pTileExtra = pTile->GetExtra<TileExtra_Lock>();
    if (!pTileExtra)
        return;

    if (!pTileExtra->HasAccess(pPlayer->GetUserID()))
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetDisplayedItem());
    if (!pItem || pItem->type != ITEM_TYPE_LOCK)
        return;

    if (pTileExtra->ownerID != pPlayer->GetUserID())
    {
        if (pTileExtra->IsAdmin(pPlayer->GetUserID()))
        {
            // not "someone" but lets keep it no need to care much it no?
            DialogBuilder db;
            db.SetDefaultColor('o')
                .AddLabelWithIcon("`wEdit " + pItem->name, pItem->id, true)
                .EmbedData("tilex", pTile->GetPos().x)
                .EmbedData("tiley", pTile->GetPos().y)
                .AddLabel("This lock is owned by someone, but I have access on it.")
                .EndDialog("lock_edit", "Remove My Access", "Cancel");
            pPlayer->SendOnDialogRequest(db.Get());
        }
        else
        {
            pPlayer->SendOnTalkBubble("I'm `4unable`` to pick the lock.", true);
        }

        return;
    }

    Vector2Int& vTilePos = pTile->GetPos();

    if (pTileExtra->GetTotalAccessedCount() == 0)
    {
        HandleFromCache(pPlayer, pWorld->GetInstanceID(), vTilePos.x, vTilePos.y);
    }
    else
    {
        GetUserCacheManager()->FetchMetadata(pPlayer->GetNetID(), CACHE_REQ_WORLD_LOCK_DIALOG, pTileExtra->accessList,
                                             {pWorld->GetInstanceID(), vTilePos.x, vTilePos.y});
    }
}

void LockDialog::HandleFromCache(GamePlayer* pPlayer, uint32 worldInstanceID, int32 tileX, int32 tileY)
{
    if (!pPlayer || pPlayer->GetCurrentWorld() != worldInstanceID)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(worldInstanceID);
    if (!pWorld)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if (!pTile)
        return;

    TileExtra_Lock* pTileExtra = pTile->GetExtra<TileExtra_Lock>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("I'm `4unable`` to pick the lock.", true);
        return;
    }

    if (pTileExtra->ownerID != pPlayer->GetUserID())
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (pItem->type != ITEM_TYPE_LOCK)
        return;

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon("`wEdit " + pItem->name, pItem->id, true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y)
        .AddLabel("`wAccess list:``")
        .AddSpacer();

    uint32 totalAcccessedCount = pTileExtra->GetTotalAccessedCount();
    if (totalAcccessedCount > 0)
    {
        UserCacheManager* pUserMgr = GetUserCacheManager();

        for (auto& id : pTileExtra->accessList)
        {
            if (id > 0 && id != pPlayer->GetUserID())
            {
                UserMetadata* pMetaData = pUserMgr->GetMetadata(id);
                if (!pMetaData)
                {
                    db.AddCheckBox("checkbox_" + ToString(id), "Error get name (#" + ToString(id) + ")", true);
                    continue;
                }

                db.AddCheckBox("checkbox_" + ToString(id), pMetaData->displayName, true);
            }
        }
    }
    else
    {
        db.AddLabel("Currently. you're the only one with access.``");
    }

    if (totalAcccessedCount < 26)
    {
        db.AddSpacer().AddPlayerPicker("playerNetID", "`wAdd``");
    }
    else
    {
        db.AddSpacer().AddLabel("`4(max players added)``");
    }

    if (pItem->id == ITEM_ID_BUILDERS_LOCK)
    {
        db.AddCheckBox("checkbox_public", "Allow anyone to Build or Break",
                       pTile->HasFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC));
    }
    else
    {
        db.AddCheckBox("checkbox_public", "Allow anyone to build", pTile->HasFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC));
    }

    if (IsWorldLock(pItem->id))
    {
        db.AddCheckBox("checkbox_disable_music", "Disable Custom Music Blocks",
                       pTileExtra->HasFlag(TILE_EXTRA_LOCK_DISABLE_MUSIC));

        if (!pTileExtra->HasFlag(TILE_EXTRA_LOCK_DISABLE_MUSIC))
        {
            int32 tempo = pTileExtra->GetTempo();
            if (tempo == -1)
            {
                tempo = 100;
                pTileExtra->SetTempo(100);
            }

            db.AddTextInput("tempo", "Music BPM", ToString(tempo), 3);
        }

        db.AddCheckBox("checkbox_disable_music_render", "Make Custom Music Blocks invisible",
                       pTileExtra->HasFlag(TILE_EXTRA_LOCK_DISABLE_RENDER_MUSIC));

        if (pItem->id == ITEM_ID_ROYAL_LOCK)
        {
            db.AddTextBox("Ye Royal Options")
                .AddCheckBox("checkbox_silence", "Silence, Peasants!", pTileExtra->HasFlag(TILE_EXTRA_LOCK_SILENCE))
                .AddCheckBox("checkbox_rainbow", "Royal Rainbows!", pTileExtra->HasFlag(TILE_EXTRA_LOCK_RAINBOW_TRAIL));
        }

        if (pPlayer->GetInventory().GetCountOfItem(ITEM_ID_WORLD_KEY) == 0)
        {
            db.AddButton("getKey", "Get World Key");
        }
    }
    else
    {
        db.AddCheckBox("checkbox_ignore", "Ignore empty air", pTileExtra->HasFlag(TILE_EXTRA_LOCK_IGNORE_EMPTY))
            .AddButton("recalcLock", "`wRe-apply lock``");

        if (pItem->id == ITEM_ID_BUILDERS_LOCK)
        {
            db.AddSpacer()
                .AddSmallText("This lock allows Building or Breaking.")
                .AddSmallText("(ONLY if \"Allow anyone to Build or Break\" is checked above)!")
                .AddSpacer()
                .AddSmallText("Leaving this box unchecked only allows Breaking.")
                .AddCheckBox("checkbox_buildonly", "Only Allow Building!",
                             pTileExtra->HasFlag(TILE_EXTRA_LOCK_BUILD_ONLY))
                .AddSmallText("People with lock access can both build and break unless you check below. The lock "
                              "owner can always build and break.")
                .AddCheckBox("checkbox_admins", "Admins Are Limited",
                             pTileExtra->HasFlag(TILE_EXTRA_LOCK_LIMIT_ADMINS));
        }
    }

    db.EndDialog("lock_edit", "OK", "Cancel");
    pPlayer->SendOnDialogRequest(db.Get());
}

void LockDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (pPlayer->GetCurrentWorld() == 0)
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
    {
        pPlayer->SendOnTalkBubble("I was looking at a lock but now it's gone.  Magic is real!", false);
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (pItem->type != ITEM_TYPE_LOCK)
        return;

    TileExtra_Lock* pTileExtra = pTile->GetExtra<TileExtra_Lock>();
    if (!pTileExtra)
        return;

    if (!pTileExtra->HasAccess(pPlayer->GetUserID()))
        return;

    if (pTileExtra->ownerID != pPlayer->GetUserID())
    {
        pTileExtra->RemoveFromList(pPlayer->GetUserID());
        pWorld->SendTileUpdate(pTile, pPlayer);
        pWorld->SendNameChangeToAll(pPlayer);
        pPlayer->PlaySFX("dialog_cancel.wav");

        pWorld->SendConsoleMessageToAll(pPlayer->GetDisplayName(true) + "`` removed their access from a " +
                                        pItem->name);
        return;
    }

    bool tileNeedsUpdate = false;

    if (auto pIgnoreAir = packet.Find("checkbox_ignore"_hash))
    {
        bool val;
        if (pIgnoreAir->GetBool(val) != TO_INT_SUCCESS)
            return;

        if (val != pTileExtra->HasFlag(TILE_EXTRA_LOCK_IGNORE_EMPTY))
        {
            tileNeedsUpdate = true;
        }

        val ? pTileExtra->SetFlag(TILE_EXTRA_LOCK_IGNORE_EMPTY) : pTileExtra->RemoveFlag(TILE_EXTRA_LOCK_IGNORE_EMPTY);
    }

    bool oldPublicFlag = pTile->HasFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC);
    if (auto pPublic = packet.Find("checkbox_public"_hash))
    {
        bool val;
        if (pPublic->GetBool(val) != TO_INT_SUCCESS)
            return;

        if (val != pTile->HasFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC))
        {
            tileNeedsUpdate = true;
        }

        val ? pTile->SetFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC) : pTile->RemoveFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC);
    }

    if (pItem->id == ITEM_ID_BUILDERS_LOCK)
    {
        if (auto pBuildOnly = packet.Find("checkbox_buildonly"_hash))
        {
            bool val;
            if (pBuildOnly->GetBool(val) != TO_INT_SUCCESS)
                return;

            if (val != pTileExtra->HasFlag(TILE_EXTRA_LOCK_BUILD_ONLY))
            {
                tileNeedsUpdate = true;
            }

            val ? pTileExtra->SetFlag(TILE_EXTRA_LOCK_BUILD_ONLY) : pTileExtra->RemoveFlag(TILE_EXTRA_LOCK_BUILD_ONLY);
        }

        if (auto pLimitAdmins = packet.Find("checkbox_admins"_hash))
        {
            bool val;
            if (pLimitAdmins->GetBool(val) != TO_INT_SUCCESS)
                return;

            if (val != pTileExtra->HasFlag(TILE_EXTRA_LOCK_LIMIT_ADMINS))
            {
                tileNeedsUpdate = true;
            }

            val ? pTileExtra->SetFlag(TILE_EXTRA_LOCK_LIMIT_ADMINS)
                : pTileExtra->RemoveFlag(TILE_EXTRA_LOCK_LIMIT_ADMINS);
        }
    }

    if (tileNeedsUpdate && IsWorldLock(pItem->id))
    {
        string notifyPublic = pPlayer->GetDisplayName(true);
        notifyPublic += " `whas set `$World Lock`w to ";
        notifyPublic += pTile->HasFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC) ? "`$PUBLIC" : "`4PRIVATE";
        pWorld->SendConsoleMessageToAll(notifyPublic);
    }

    if (IsWorldLock(pItem->id))
    {
        if (auto pDisableMusic = packet.Find("checkbox_disable_music"_hash))
        {
            bool val;
            if (pDisableMusic->GetBool(val) != TO_INT_SUCCESS)
                return;

            val ? pTileExtra->SetFlag(TILE_EXTRA_LOCK_DISABLE_MUSIC)
                : pTileExtra->RemoveFlag(TILE_EXTRA_LOCK_DISABLE_MUSIC);
        }

        if (auto pDisableMusicRender = packet.Find("checkbox_disable_music_render"_hash))
        {
            bool val;
            if (pDisableMusicRender->GetBool(val) != TO_INT_SUCCESS)
                return;

            val ? pTileExtra->SetFlag(TILE_EXTRA_LOCK_DISABLE_RENDER_MUSIC)
                : pTileExtra->RemoveFlag(TILE_EXTRA_LOCK_DISABLE_RENDER_MUSIC);
        }

        if (auto pTempo = packet.Find("tempo"_hash))
        {
            int32 tempo = 100;
            if (pTempo->GetInt(tempo) != TO_INT_SUCCESS)
            {
                tempo = 100;
            }

            if (tempo < 0)
            {
                tempo = 20;
                pPlayer->SendOnTalkBubble("Tempo must be from 20-200 BPM.", false);
            }

            if (tempo > 200)
            {
                tempo = 200;
                pPlayer->SendOnTalkBubble("Tempo must be from 20-200 BPM.", false);
            }

            int32 currentTempo = pTileExtra->GetTempo();
            if ((currentTempo == -1 && tempo != 100) || (currentTempo != -1 && currentTempo != tempo))
            {
                pTileExtra->SetTempo(tempo);
                tileNeedsUpdate = true;
            }
        }

        if (pItem->id == ITEM_ID_ROYAL_LOCK)
        {
            if (auto pSilenced = packet.Find("checkbox_silence"_hash))
            {
                bool val;
                if (pSilenced->GetBool(val) != TO_INT_SUCCESS)
                    return;

                if (val != pTileExtra->HasFlag(TILE_EXTRA_LOCK_SILENCE))
                {
                    tileNeedsUpdate = true;
                }

                if (val)
                {
                    pTileExtra->SetFlag(TILE_EXTRA_LOCK_SILENCE);
                    pWorld->SendTalkBubbleAndConsoleToAll(
                        "`w" + pPlayer->GetDisplayName(true) + "```w has silenced the peasants!``", false);
                }
                else
                {
                    pTileExtra->RemoveFlag(TILE_EXTRA_LOCK_SILENCE);
                    pWorld->SendTalkBubbleAndConsoleToAll(
                        "`w" + pPlayer->GetDisplayName(true) + "```w has allowed the peasants to speak.``", false);
                }
            }

            if (auto pTrail = packet.Find("checkbox_rainbow"_hash))
            {
                bool val;
                if (pTrail->GetBool(val) != TO_INT_SUCCESS)
                    return;

                if (val != pTileExtra->HasFlag(TILE_EXTRA_LOCK_RAINBOW_TRAIL))
                {
                    tileNeedsUpdate = true;
                }

                val ? pTileExtra->SetFlag(TILE_EXTRA_LOCK_RAINBOW_TRAIL)
                    : pTileExtra->RemoveFlag(TILE_EXTRA_LOCK_RAINBOW_TRAIL);
            }

            tileNeedsUpdate = true;
        }
    }

    if (tileNeedsUpdate)
    {
        pWorld->SendTileUpdate(pTile);
    }

    if (auto pPlayerNetID = packet.Find("playerNetID"_hash))
    {
        int32 targetPlayerUserID = 0;
        if (pPlayerNetID->GetInt(targetPlayerUserID) != TO_INT_SUCCESS)
            return;

        GamePlayer* pTarget = GetPlayerManager()->GetPlayerByNetID(targetPlayerUserID);
        if (!pTarget)
        {
            pPlayer->SendOnTalkBubble("Unable to add person to lock. Try again.", false);
            return;
        }

        if (pTileExtra->ownerID == targetPlayerUserID)
        {
            pPlayer->SendOnTalkBubble("I already have access!", false);
            return;
        }

        if (pTileExtra->HasAccess(targetPlayerUserID))
        {
            pPlayer->SendOnTalkBubble(pTarget->GetRawName() + " already has access to the lock", false);
            return;
        }

        if (pTileExtra->GetTotalAccessedCount() > 25)
        {
            pPlayer->SendOnTalkBubble("Unable to add, the lock is full!", false);
            return;
        }

        pTarget->SendLockAccessRequest(pPlayer, pTile);
        return;
    }

    const std::string_view accPrefix = "checkbox_";
    UserCacheManager* pUserMgr = GetUserCacheManager();

    for (uint8 i = 0; i < packet.count; ++i)
    {
        if (packet.fields[i].keySize == 0)
            continue;

        std::string_view key = packet.fields[i].GetKeyStringView();
        if (key.size() <= accPrefix.size() || key.substr(0, accPrefix.size()) != accPrefix)
            continue;

        std::string_view numStr = key.substr(accPrefix.size());
        uint32 targetUserID = 0;
        if (ToUInt(numStr.data(), numStr.size(), targetUserID) != TO_INT_SUCCESS)
            continue;

        bool isChecked;
        if (packet.fields[i].GetBool(isChecked) != TO_INT_SUCCESS || isChecked)
            continue;

        pTileExtra->RemoveFromList(targetUserID);

        GamePlayer* pTarget = GetPlayerManager()->GetPlayerByUserID(targetUserID);
        if (!pTarget)
        {
            UserMetadata* pUserMeta = pUserMgr->GetMetadata(targetUserID);
            if (!pUserMeta)
                continue;

            pWorld->SendConsoleMessageToAll(pUserMeta->displayName + "`w was removed from a " + pItem->name + "`w.");
        }
        else
        {
            if (pPlayer->GetCurrentWorld() == pTarget->GetCurrentWorld())
            {
                pWorld->SendTileUpdate(pTile);
            }

            pTarget->SendOnTalkBubble(pPlayer->GetDisplayName(true) +
                                          "`w has `4removed`w your access from a lock on world `w" +
                                          pWorld->GetWorlName() + "`w.",
                                      false);
            pTarget->PlaySFX("dialog_cancel.wav");

            pWorld->SendNameChangeToAll(pTarget);
            pWorld->SendConsoleMessageToAll(pPlayer->GetDisplayName(false) + "`o was removed from a " + pItem->name +
                                            "`o.");
        }
    }

    auto pButtonClicked = packet.Find("buttonClicked"_hash);
    if (!pButtonClicked)
        return;

    if (pButtonClicked->valueSize == 0 || pButtonClicked->valueSize > 20)
        return;

    std::string_view clickedButton = pButtonClicked->GetStringView();

    if (clickedButton == "recalcLock")
    {
        if (IsWorldLock(pItem->id))
            return;

        std::vector<TileInfo*> lockedTiles;
        bool lockSuccsess = pWorld->GetTileManager()->ApplyLockTiles(
            pTile, GetMaxTilesToLock(pItem->id), pTileExtra->HasFlag(TILE_EXTRA_LOCK_IGNORE_EMPTY), lockedTiles);

        if (!lockSuccsess)
        {
            pPlayer->SendOnTalkBubble("Something went wrong, unable to re-calc lock.", true);
            return;
        }
        else
        {
            pWorld->SendLockPacketToAll(pTileExtra->ownerID, pItem->id, lockedTiles, pTile);
        }

        pWorld->SendPlayPositionedToAll(pPlayer, "use_lock.wav");
        return;
    }

    if (clickedButton == "getKey" && IsWorldLock(pItem->id))
    {

        // add floating & untradeable check

        if (pTileExtra->GetTotalAccessedCount() != 0)
        {
            pPlayer->SendOnTalkBubble("`4You'll first need to remove all co-owners`` from your `5World Lock`` to get a "
                                      "`#World Key`` to trade this world.",
                                      false);
        }
        else
        {
            PlayerInventory& inventory = pPlayer->GetInventory();
            if (inventory.GetCountOfItem(ITEM_ID_WORLD_KEY) != 0)
            {
                pPlayer->SendOnTalkBubble("`4Looks like you already have the key!", false);
                return;
            }

            pPlayer->SendOnTalkBubble("You got a `#World Key``! You can now trade this world to other players.", false);
            pPlayer->ModifyInventoryItem(ITEM_ID_WORLD_KEY, 1);
            pPlayer->PlaySFX("use_lock.wav");
        }

        return;
    }
}

void MailboxBlockDialog::Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem)
{
    if (!pPlayer || !pTile || !pItem)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    TileExtra_Mailbox* pTileExtra = pTile->GetExtra<TileExtra_Mailbox>();
    if (!pTileExtra)
        return;

    if (pItem->type != ITEM_TYPE_MAILBOX)
        return;

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon(pItem->name, pItem->id, true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y);

    if (pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
    {
        if (pTileExtra->letters.empty())
        {
            db.AddTextBox("Your mailbox is currently empty.")
                .AddTextBox("Write a letter to yourself?")
                .AddTextInput("sign_text", "", "", 128)
                .AddSpacer()
                .AddButton("send", "`2Send Letter``");
        }
        else
        {
            std::vector<int32> userIDs;
            userIDs.reserve(pTileExtra->letters.size());

            for (auto& letter : pTileExtra->letters)
            {
                userIDs.push_back(letter.userID);
            }

            GetUserCacheManager()->FetchMetadata(pPlayer->GetNetID(), CACHE_REQ_MAILBOX_BLOCK, userIDs,
                                                 {pWorld->GetInstanceID(), vTilePos.x, vTilePos.y});
        }
    }
    else
    {
        bool canWrite = true;

        if (pTileExtra->letters.size() >= 20)
        {
            db.AddTextBox("This mailbox already has `w" + ToString(pTileExtra->letters.size()) +
                          "`` letters in it. Try again later.");
            canWrite = false;
        }

        if (pTileExtra->HasLetterFromID(pPlayer->GetUserID()))
        {
            if (canWrite)
            {
                db.AddTextBox("You've already crammed `w1`` of your letters into the mailbox, better wait.");
            }
        }
        else if (canWrite)
        {
            db.AddTextBox("Want to leave a message for the owner?")
                .AddTextInput("sign_text", "", "", 128)
                .AddSpacer()
                .AddButton("send", "`2Send Letter``");
        }
    }

    db.EndDialog("mailbox_edit", "", "Cancel");
    pPlayer->SendOnDialogRequest(db.Get());
}

void MailboxBlockDialog::HandleFromCache(GamePlayer* pPlayer, uint32 worldInstanceID, int32 tileX, int32 tileY)
{
    if (!pPlayer || pPlayer->GetCurrentWorld() != worldInstanceID)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(worldInstanceID);
    if (!pWorld)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if (!pTile)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (pItem->type != ITEM_TYPE_MAILBOX)
        return;

    TileExtra_Mailbox* pTileExtra = pTile->GetExtra<TileExtra_Mailbox>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The mailbox is gone!.", false);
        return;
    }

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon(pItem->name, pItem->id, true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y)
        .AddTextBox("You have `w" + ToString(pTileExtra->letters.size()) + "`` letter" +
                    (pTileExtra->letters.size() > 1 ? "s:" : ":"))
        .AddSpacer();

    UserCacheManager* pUserMgr = GetUserCacheManager();

    for (auto& letter : pTileExtra->letters)
    {
        UserMetadata* pMetaData = pUserMgr->GetMetadata(letter.userID);

        db.AddLabelWithIcon("`#\"" + letter.message + "\" - `w" +
                                (pMetaData ? pMetaData->displayName : ToString(letter.userID)),
                            ITEM_ID_LETTER)
            .AddSpacer();
    }

    if (pTileExtra->letters.size() >= 20)
    {
        db.AddTextBox("This mailbox already has `w" + ToString(pTileExtra->letters.size()) +
                      "`` letters in it, can't add more until you clear them.");
    }
    else
    {
        db.AddTextBox("Write a letter to yourself?")
            .AddTextInput("sign_text", "", "", 128)
            .AddSpacer()
            .AddButton("send", "`2Send Letter``");
    }

    db.AddSpacer().AddButton("clear", "`4Empty Mailbox``").EndDialog("mailbox_edit", "", "Cancel");

    pPlayer->SendOnDialogRequest(db.Get());
}

void MailboxBlockDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pTileX = packet.Find("tilex"_hash);
    if (!pTileX)
        return;

    auto pTileY = packet.Find("tiley"_hash);
    if (!pTileY)
        return;

    auto pButtonClicked = packet.Find("buttonClicked"_hash);
    if (!pButtonClicked)
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

    TileExtra_Mailbox* pTileExtra = pTile->GetExtra<TileExtra_Mailbox>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The mailbox is gone!", false);
        return;
    }

    std::string_view buttonClicked = pButtonClicked->GetStringView();

    if (buttonClicked == "send")
    {
        auto pSignText = packet.Find("sign_text"_hash);
        if (!pSignText)
            return;

        if (pSignText->valueSize > 128)
        {
            pPlayer->SendOnTalkBubble("That letter is too long!", false);
            return;
        }

        string text = pSignText->GetString();
        RemoveExtraWhiteSpaces(text);
        RemoveGTColorCodes(text);

        if (text.empty() || text.size() > 128)
            return;

        if (text.size() < 3)
        {
            pPlayer->SendOnTalkBubble("That's not interesting enough to mail.", false);
            return;
        }

        if (pTileExtra->letters.size() >= 20)
        {
            pPlayer->SendOnTalkBubble("You aren't able to fit another letter inside, it's jammed full.", false);
            return;
        }

        if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile) && pTileExtra->HasLetterFromID(pPlayer->GetUserID()))
        {
            pPlayer->SendOnTalkBubble("Don't flood the mailbox.", false);
            return;
        }

        uint32 totalStrLen = text.size();
        for (auto& letter : pTileExtra->letters)
        {
            totalStrLen += letter.message.size();
        }

        if (totalStrLen > 1024)
        {
            LOGGER_LOG_ERROR("Failed to write into mailbox totalStrLen (with text): %d, text size: %d, userID: %d",
                             totalStrLen, text.size(), pPlayer->GetUserID());
            return;
        }

        pTileExtra->letters.push_back({pPlayer->GetUserID(), text});

        pPlayer->SendOnTalkBubble("`2You placed your letter in the mailbox.``", false);
        pPlayer->PlaySFX("page_turn.wav");

        if (pTileExtra->letters.size() == 1)
        {
            pTile->SetFlag(TILE_FLAG_IS_ON);
            pWorld->SendTileUpdate(pTile);
        }

        return;
    }

    if (buttonClicked == "clear")
    {
        if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        {
            pPlayer->SendOnTalkBubble("You can't figure out how to open it.", false);
            return;
        }

        if (pTileExtra->letters.size() > 0)
        {
            pTile->RemoveFlag(TILE_FLAG_IS_ON);
            pWorld->SendTileUpdate(pTile);
        }

        pTileExtra->letters.clear();

        pPlayer->SendOnTalkBubble("`2Mailbox emptied.``", false);
        pPlayer->PlaySFX("page_turn.wav");
        return;
    }
}

void MannequinDialog::Request(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile)
        return;

    TileExtra_Mannequin* pTileExtra = pTile->GetExtra<TileExtra_Mannequin>();
    if (!pTileExtra)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem)
        return;

    if (pItem->type != ITEM_TYPE_MANNEQUIN)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        return;

    if (pTileExtra->text.find("__&%@PL@%&__") != string::npos)
    {
        pTileExtra->text = "";
    }

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon("`wEdit " + pItem->name + "``", pItem->id, true)
        .AddTextBox("To dress, select a clothing item then use on the mannequin. To remove clothes, punch it or select "
                    "which item "
                    "to remove.<CR><CR>It will go into your backpack if you have room.");

    bool hasAnyClothing = false;

    for (int32 i = 0; i < 9; ++i)
    {
        if (pTileExtra->clothes[i] == ITEM_ID_BLANK)
            continue;

        ItemInfo* pCloth = GetItemInfoManager()->GetItemByID(pTileExtra->clothes[i]);
        if (!pCloth)
            continue;

        db.AddCheckBox("checkbox", pCloth->name, false);
        hasAnyClothing = true;
    }

    if (hasAnyClothing)
    {
        db.AddSpacer()
            .AddButton("clear_selected", "`4Remove Selected Items``")
            .AddButton("clear", "`4Remove All Items``");
    }

    Vector2Int& vTilePos = pTile->GetPos();
    db.AddTextBox("<CR><CR>What would you like to write on its sign?``")
        .AddTextInput("sign_text", "", pTileExtra->text, 128)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y)
        .EndDialog("mannequin_edit", "OK", "Cancel");

    pPlayer->SendOnDialogRequest(db.Get());
}

void MannequinDialog::RequestPutItem(GamePlayer* pPlayer, TileInfo* pTile, int32 itemID, bool fromDialog)
{
    if (!pPlayer || !pTile)
        return;

    TileExtra_Mannequin* pTileExtra = pTile->GetExtra<TileExtra_Mannequin>();
    if (!pTileExtra)
        return;

    if (pPlayer->GetDistToTileInTiles(pTile) >= 4)
    {
        pPlayer->SendOnTalkBubble("`5[`2I'll need to get closer``]``", false);
        return;
    }

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem)
        return;

    if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
    {
        pPlayer->SendOnTalkBubble("`5[`2That " + pItem->name + " isn't yours!``]``", false);
        return;
    }

    if (!pWorld->GetTileManager()->IsTileLockedWithLock(pTile))
    {
        pPlayer->SendOnTalkBubble("`5[`2It's not safe to use an unlocked " + pItem->name + " !``]``", false);
        return;
    }

    if (pTile->GetBG() == ITEM_ID_DARK_CAVE_BACKGROUND)
    {
        if (!pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
        {
            pPlayer->SendOnTalkBubble("`5[`2It's too dark to use this " + pItem->name + " !``]``", false);
            return;
        }
    }

    ItemInfo* pCloth = GetItemInfoManager()->GetItemByID(itemID);
    if (!pCloth)
        return;

    if (pCloth->type != ITEM_TYPE_CLOTHES)
    {
        pPlayer->SendOnTalkBubble(
            "`5[`2That would just be weird.  Try putting clothes on your " + pItem->name + " instead!``]``", false);
        return;
    }

    if (pItem->id == ITEM_ID_SNOWTOPIAN && (pCloth->bodyPart == BODY_PART_PANT || pCloth->bodyPart == BODY_PART_SHOE))
    {
        static string partNames[9] = {"Hat",    "Shirts",   "Pants", "Shoes",      "Face Items",
                                      "Gloves", "Backpack", "Hairs", "Chest Items"};

        pPlayer->SendOnTalkBubble("`5[`2A " + pItem->name + " cannot wear " + partNames[pCloth->bodyPart] + "!``]``",
                                  false);
        return;
    }

    if (pCloth->HasFlag(ITEM_FLAG_UNTRADEABLE))
    {
        pPlayer->SendOnTalkBubble("`5[`2That item is just too valuable to part with``]``", false);
        return;
    }

    if (pPlayer->GetInventory().GetCountOfItem(pCloth->id) < 1)
    {
        pPlayer->SendOnTalkBubble("`5[`2You don't have that item anymore``]``", false);
        return;
    }

    int32 currentEquipped = pTileExtra->clothes[pCloth->bodyPart];
    if (currentEquipped == pCloth->id)
    {
        pPlayer->SendOnTalkBubble("`5[`2You giggle as you swap two identical items``]``", false);
        return;
    }

    if (fromDialog)
    {
        pTileExtra->SetCloth(pCloth->bodyPart, pCloth->id);

        if (pCloth->bodyPart == BODY_PART_HAIR)
        {
            // todo hair color
        }

        pTileExtra->charFlags = pPlayer->GetCharData().charState;
        pTileExtra->char2Flags = pPlayer->GetCharData().char2State;

        pPlayer->ModifyInventoryItem(pCloth->id, -1);
        pWorld->SendPlayPositionedToAll(pPlayer, "change_clothes.wav");

        if (currentEquipped != ITEM_ID_BLANK)
        {
            pPlayer->ModifyInventoryItem(currentEquipped, 1);
            // todo drop
        }

        pWorld->SendTileUpdate(pTile);
    }
    else
    {
        Vector2Int& vTilePos = pTile->GetPos();

        DialogBuilder db;
        db.SetDefaultColor('o')
            .AddLabelWithIcon("`w" + pCloth->name, pItem->id, true)
            .AddTextBox("Do you really want to put your " + pCloth->name + " on the " + pItem->name + "?")
            .EmbedData("put", pCloth->id)
            .EmbedData("tilex", vTilePos.x)
            .EmbedData("tiley", vTilePos.y)
            .EndDialog("mannequin_edit", "Yes", "No");

        pPlayer->SendOnDialogRequest(db.Get());
    }
}

bool MannequinDialog::RequestRemoveItem(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile)
        return false;

    TileExtra_Mannequin* pTileExtra = pTile->GetExtra<TileExtra_Mannequin>();
    if (!pTileExtra)
        return false;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return false;

    if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        return false;

    int32 clothIndex = -1;

    for (int32 i = 0; i < 9; ++i)
    {
        if (pTileExtra->clothes[i] != ITEM_ID_BLANK)
        {
            clothIndex = i;
            break;
        }
    }

    if (clothIndex == -1)
        return false;

    if (pPlayer->GetInventory().GetFitItemCount(pTileExtra->clothes[clothIndex]) < 1)
    {
        pPlayer->SendOnTalkBubble("You don't have room for the item!", false);
        return true;
    }

    pTileExtra->SetCloth(clothIndex, ITEM_ID_BLANK);
    pPlayer->ModifyInventoryItem(pTileExtra->clothes[clothIndex], 1);

    pWorld->SendPlayPositionedToAll(pPlayer, "change_clothes.wav");
    pWorld->SendTileUpdate(pTile);
    return true;
}

void MannequinDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
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

    TileExtra_Mannequin* pTileExtra = pTile->GetExtra<TileExtra_Mannequin>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The mannequin is gone!", false);
        return;
    }

    if (auto pPut = packet.Find("put"_hash))
    {
        int32 itemID = 0;
        if (pPut->GetInt(itemID) != TO_INT_SUCCESS)
            return;

        RequestPutItem(pPlayer, pTile, itemID, true);
        return;
    }

    if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        return;

    bool tileNeedsUpdate = false;

    if (auto pButtonClicked = packet.Find("buttonClicked"_hash))
    {
        const std::string_view buttonClicked = pButtonClicked->GetStringView();

        if (buttonClicked == "clear" || buttonClicked == "clear_selected")
        {
            std::string_view prefix = "checkbox";
            int32 clothIndex = 0;

            for (uint8 i = 0; i < packet.count; ++i)
            {
                if (packet.fields[i].keySize == 0)
                    continue;

                std::string_view key = packet.fields[i].GetKeyStringView();
                if (key.size() != prefix.size() || key != prefix)
                    continue;

                while (clothIndex < 9 && pTileExtra->clothes[clothIndex] == 0)
                    clothIndex++;

                if (clothIndex >= 9)
                    break;

                bool isChecked;
                if (packet.fields[i].GetBool(isChecked) != TO_INT_SUCCESS)
                    continue;

                if (isChecked || buttonClicked == "clear")
                {
                    ItemInfo* pCloth = GetItemInfoManager()->GetItemByID(pTileExtra->clothes[clothIndex]);
                    if (!pCloth)
                    {
                        clothIndex++;
                        continue;
                    }

                    bool invHasItem = pPlayer->GetInventory().GetCountOfItem(pCloth->id) > 0;
                    bool canItemFit = pPlayer->GetInventory().GetFitItemCount(pCloth->id) > 0;

                    if (invHasItem && !canItemFit)
                    {
                        pPlayer->SendOnTalkBubble(
                            "Hmm, I'll need to drop the " + pCloth->name +
                                " items I already have before I can get the ones in mannequin, they won't fit.",
                            false);
                    }

                    if (!invHasItem && !canItemFit)
                    {
                        pPlayer->SendOnTalkBubble("I don't have enough room in my backpack to get `5" + pCloth->name +
                                                      " ``from the mannequin!",
                                                  false);
                    }

                    if (canItemFit)
                    {
                        pPlayer->ModifyInventoryItem(pCloth->id, 1);
                        tileNeedsUpdate = true;
                        pTileExtra->SetCloth(clothIndex, ITEM_ID_BLANK);
                    }
                }

                clothIndex++;
            }
        }
    }

    if (tileNeedsUpdate)
    {
        pWorld->SendTileUpdate(pTile);
    }

    if (auto pSignText = packet.Find("sign_text"_hash))
    {
        if (pSignText->valueSize > 258)
            return;

        const std::string_view signText = pSignText->GetStringView();

        if (signText.find("__&%@PL@%&__") != string::npos)
            return;

        if (signText != pTileExtra->text)
        {
            pTileExtra->text = signText;
            pWorld->SendTileUpdate(pTile);
        }
    }
}

void OuijaBoardDialog::RequestMain(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile)
        return;

    TileExtra_OuijaBoard* pTileExtra = pTile->GetExtra<TileExtra_OuijaBoard>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Woops, its gone.", false);
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem)
        return;

    if (pItem->type != ITEM_TYPE_OUIJA_BOARD)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    bool isDarkSpiritBoard = pTile->GetFG() == ITEM_ID_DARK_SPIRIT_BOARD;

    if (pTileExtra->items.empty() && !pWorld->GetTileManager()->RandomizeOuijaBoardTile(pTile))
    {
        pPlayer->SendOnTalkBubble("Opps, something happened badly.", false);
        LOGGER_LOG_WARN("Tried to generate ouija but it failed!? isDarkSpirit: %d, player: %d world: %d",
                        isDarkSpiritBoard, pPlayer->GetNetID(), pWorld->GetInstanceID());
        return;
    }

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon("`w" + pItem->name, pItem->id, true)
        .AddLabel("You sense a presence in the room...");

#ifndef _DEBUG
    RectFloat tileRect = pTile->GetRect();
    tileRect.InFlate(32 * 2);

    if (pWorld->GetPlayersInWorldRect(tileRect).size() < 2)
    {
        db.AddTextBox("The planchette wobbles and then remains still...")
            .AddTextBox("Perhaps you need more people to help.")
            .EndDialog("ouijaboard", "", "Close");
        pPlayer->SendOnDialogRequest(db.Get());
        return;
    }
#endif

    int32 ouijaType = 0;
    if (ToInt(pTileExtra->ouijaType, ouijaType) != TO_INT_SUCCESS)
        return;

    db.AddTextBox("The planchette slowly comes to life and begins to drift across the board...")
        .AddSpacer()
        .AddTextBox("Ask your questions")
        .AddButton("showOuijaBoardItems", "What do you require of us?")
        .AddButton("showOuijaBoardCommand", "How can we bring you here?");

    if (ouijaType == 1)
    {
        db.AddSpacer()
            .AddSmallText("A small inscription is etched into the bottom of the board!")
            .AddSmallText("`4HE IS TOO POWERFUL! HE WILL DESTROY YOUR WORLD!``");
    }

    db.AddButton("adminTrigger", "`4Trigger");

    Vector2Int& vTilePos = pTile->GetPos();
    db.AddSpacer().EmbedData("tilex", vTilePos.x).EmbedData("tiley", vTilePos.y).EndDialog("ouijaboard", "", "Close");

    pPlayer->SendOnDialogRequest(db.Get());
}

void OuijaBoardDialog::RequestItemInfo(GamePlayer* pPlayer, TileInfo* pTile, int32 itemIndex, eOuijaItemInfoType type)
{
    if (!pPlayer || !pTile || itemIndex < -1 || itemIndex >= 2)
        return;

    TileExtra_OuijaBoard* pTileExtra = pTile->GetExtra<TileExtra_OuijaBoard>();
    if (!pTileExtra)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    if (pTileExtra->items.empty() && !pWorld->GetTileManager()->RandomizeOuijaBoardTile(pTile))
    {
        pPlayer->SendOnTalkBubble("Opps, something happened badly.", false);
        return;
    }

    if (pTileExtra->items.size() % 3 != 0 || pTileExtra->items.size() / 3 < 1 ||
        (itemIndex != -1 && (itemIndex + 1 > pTileExtra->items.size() / 3)))
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem)
        return;

    if (pItem->type != ITEM_TYPE_OUIJA_BOARD)
        return;

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon("`w" + pItem->name, pItem->id, true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y);

#ifndef _DEBUG
    RectFloat tileRect = pTile->GetRect();
    tileRect.InFlate(32 * 2);

    if (pWorld->GetPlayersInWorldRect(tileRect).size() < 2)
    {
        db.AddTextBox("You sense a presence in the room...")
            .AddTextBox("The planchette wobbles and then remains still...")
            .AddTextBox("Perhaps you need more people to help.")
            .EndDialog("ouijaboard", "", "Close");
        pPlayer->SendOnDialogRequest(db.Get());
        return;
    }
#endif

    if (itemIndex == -1)
    {
        if (pTileExtra->items.size() / 2 == 1)
        {
            db.AddTextBox("I need a specific item to be worn to build up enough energy for me to come over.")
                .AddButton("showOuijaBoardItems1", "What item do we need?");
        }
        else
        {
            db.AddTextBox("I need 2 specific items to be worn to build up enough energy for me to come over.")
                .AddButton("showOuijaBoardItems1", "What is the first item?")
                .AddButton("showOuijaBoardItems2", "What is the second item?");
        }

        db.AddSpacer().AddButton("backToOuijaBoard", "Back").EndDialog("ouijaboard", "", "Goodbye");
        pPlayer->SendOnDialogRequest(db.Get());
        return;
    }

    ItemInfo* pOuijaItem = GetItemInfoManager()->GetItemByID(pTileExtra->items[itemIndex * 2 + 1]);
    if (!pItem)
        return;

    string itemButtonID = ToString(itemIndex + 1);

    switch (type)
    {
        case eOuijaItemInfoType::MAIN:
        {
            db.AddTextBox("I can't remember exactly what item is it. Maybe you can help jog my memory...")
                .AddSpacer()
                .AddButton("showOuijaBoardItems" + itemButtonID + "name", "What is the item's name?")
                .AddButton("showOuijaBoardItems" + itemButtonID + "amount", "How many do you need?")
                .AddButton("showOuijaBoardItems" + itemButtonID + "rarity", "What rarity is it?")
                .AddButton("showOuijaBoardItems" + itemButtonID + "seed", "How is it made?")
                .AddSpacer()
                .AddButton("showOuijaBoardItems", "Back");
            break;
        }

        case eOuijaItemInfoType::NAME:
        {
            if (pOuijaItem->name.empty())
                return;

            string nameBox = "I remember that it's name starts with `5\"";
            nameBox += pOuijaItem->name[0];
            nameBox += "\"``.";

            db.AddTextBox(nameBox);
            break;
        }

        case eOuijaItemInfoType::AMOUNT:
        {
            db.AddTextBox("To generate enough energy, `5" + ToString(pTileExtra->items[itemIndex * 2 + 2]) +
                          "`` of you need to be wearing it.");
            break;
        }

        case eOuijaItemInfoType::RARITY:
        {
            db.AddTextBox("I remember that it has a rarity of `5" + ToString(pOuijaItem->rarity) + "``.");
            break;
        }

        case eOuijaItemInfoType::SEED:
        {
            if (pOuijaItem->seed1 == 0 && pOuijaItem->seed2 == 0)
            {
                db.AddTextBox("I remember that it can't be spliced.");
                break;
            }

            ItemInfo* pSeed = GetItemInfoManager()->GetItemByID(pOuijaItem->seed1);
            if (!pSeed)
            {
                db.AddTextBox("I lost mind...");
                break;
            }

            db.AddTextBox("I remember that it can be grown by splicing `5" + pSeed->name + "`` with something.");
            break;
        }
    }

    if (type != eOuijaItemInfoType::MAIN)
    {
        db.AddSpacer().AddButton("showOuijaBoardItems" + itemButtonID, "Back");
    }

    db.EndDialog("ouijaboard", "", "Goodbye");
    pPlayer->SendOnDialogRequest(db.Get());
}

void OuijaBoardDialog::RequestCommand(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile)
        return;

    TileExtra_OuijaBoard* pTileExtra = pTile->GetExtra<TileExtra_OuijaBoard>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Woops, its gone.", false);
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem)
        return;

    if (pItem->type != ITEM_TYPE_OUIJA_BOARD)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    if (pTileExtra->items.empty() && !pWorld->GetTileManager()->RandomizeOuijaBoardTile(pTile))
    {
        pPlayer->SendOnTalkBubble("Opps, something happened badly.", false);
        return;
    }

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon("`w" + pItem->name, pItem->id, true)
        .AddTextBox("The mood in the room needs to match mine...")
        .AddTextBox("...");

#ifndef _DEBUG
    RectFloat tileRect = pTile->GetRect();
    tileRect.InFlate(32 * 2);

    if (pWorld->GetPlayersInWorldRect(tileRect).size() < pTileExtra->playerCount)
    {
        db.AddTextBox("But there arn't enough people around.").EndDialog("ouijaboard", "", "Close");
        pPlayer->SendOnDialogRequest(db.Get());
        return;
    }
#endif

    static std::unordered_map<string, string> commandLabel = {
        {"/cry", "and I'm so sad I could cry..."},
        {"/furious", "and I'm so mad I could scream..."},
        {"/rolleyes", "and you're unbelievable. I mean, really?! Come on..."},
        {"/omg", "and I'm SOOOOOOOoooo hyped! ..."},
        {"/wave", "and I feel like greeting someone..."},
        {"/dance", "and I just want to party..."},
        {"/love", "and I feel the love.."},
        {"/sleep", "and I'm so sleepy..."},
        {"/fp", "and I can't believe what just happened. I mean,  WHY?..."}};

    auto it = commandLabel.find(pTileExtra->command);
    if (it != commandLabel.end())
    {
        db.AddLabel(it->second);
    }
    else
    {
        db.AddLabel("Woah... I just lost my mood");
    }

    db.AddSpacer().AddButton("backToOuijaBoard", "Back").EndDialog("ouijaboard", "", "Close");
    pPlayer->SendOnDialogRequest(db.Get());
}

void OuijaBoardDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pButtonClicked = packet.Find("buttonClicked"_hash);
    if (!pButtonClicked || (pButtonClicked && pButtonClicked->valueSize == 0))
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

    TileExtra_OuijaBoard* pTileExtra = pTile->GetExtra<TileExtra_OuijaBoard>();
    if (!pTileExtra)
        return;

    uint32 clickedButtonHash = HashString(pButtonClicked->value, pButtonClicked->valueSize);

    switch (clickedButtonHash)
    {
        case "backToOuijaBoard"_hash:
        {
            RequestMain(pPlayer, pTile);
            return;
        }

        case "showOuijaBoardItems"_hash:
        {
            RequestItemInfo(pPlayer, pTile, -1, eOuijaItemInfoType::MAIN);
            return;
        }

        case "showOuijaBoardItems1"_hash:
        {
            RequestItemInfo(pPlayer, pTile, 0, eOuijaItemInfoType::MAIN);
            return;
        }

        case "showOuijaBoardItems1name"_hash:
        {
            RequestItemInfo(pPlayer, pTile, 0, eOuijaItemInfoType::NAME);
            return;
        }

        case "showOuijaBoardItems1amount"_hash:
        {
            RequestItemInfo(pPlayer, pTile, 0, eOuijaItemInfoType::AMOUNT);
            return;
        }

        case "showOuijaBoardItems1rarity"_hash:
        {
            RequestItemInfo(pPlayer, pTile, 0, eOuijaItemInfoType::RARITY);
            return;
        }

        case "showOuijaBoardItems1seed"_hash:
        {
            RequestItemInfo(pPlayer, pTile, 0, eOuijaItemInfoType::SEED);
            return;
        }

        case "showOuijaBoardItems2"_hash:
        {
            RequestItemInfo(pPlayer, pTile, 1, eOuijaItemInfoType::MAIN);
            return;
        }

        case "showOuijaBoardItems2name"_hash:
        {
            RequestItemInfo(pPlayer, pTile, 1, eOuijaItemInfoType::NAME);
            return;
        }

        case "showOuijaBoardItems2amount"_hash:
        {
            RequestItemInfo(pPlayer, pTile, 1, eOuijaItemInfoType::AMOUNT);
            return;
        }

        case "showOuijaBoardItems2rarity"_hash:
        {
            RequestItemInfo(pPlayer, pTile, 1, eOuijaItemInfoType::RARITY);
            return;
        }

        case "showOuijaBoardItems2seed"_hash:
        {
            RequestItemInfo(pPlayer, pTile, 1, eOuijaItemInfoType::SEED);
            return;
        }

        case "showOuijaBoardCommand"_hash:
        {
            RequestCommand(pPlayer, pTile);
            return;
        }

        case "adminTrigger"_hash:
        {
            pWorld->TriggerOuijaBoard({pPlayer}, pTile);
            return;
        }
    }
}

void PopupDialog::RequestOther(GamePlayer* pPlayer, GamePlayer* pTarget)
{
    if (!pPlayer || !pTarget)
        return;

    if (pPlayer->GetCurrentWorld() != 0 && pPlayer->GetCurrentWorld() != pTarget->GetCurrentWorld())
    {
        pPlayer->SendOnTalkBubble("Hmm, that person left.", false);
        return;
    }

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pTarget->GetCurrentWorld());
    if (!pWorld)
        return;

    PlayerProgress& progressData = pTarget->GetProgressData();
    PlayerLoginDetail& loginDetail = pTarget->GetLoginDetail();
    CharacterData& characterData = pTarget->GetCharData();
    PlayerInventory& inventory = pTarget->GetInventory();
    PlayerPlayModController& modController = pTarget->GetModController();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .EmbedData("netID", pTarget->GetNetID())
        .AddLabelWithIcon("`w" + pTarget->GetDisplayName(true) + "`` (`2" + ToString(pTarget->GetPlayerLevel()) + "``)",
                          ITEM_ID_FIST, true)
        .AddSpacer()
        .AddButton("trade", "`wTrade``");

    if (inventory.GetClothByPart(BODY_PART_HAND) == ITEM_ID_BATTLE_LEASH)
        db.AddButton("pets", "`wBattle Pets");
    else
        db.AddTextBox("(No Battle Leash equipped)");

    if (pPlayer->AbleToWorldKickOrPullSomeone(pTarget))
        db.AddButton("kick", "`4Kick``").AddButton("Pull", "`5Pull``");

    if (pPlayer->AbleToWorldBanSomeone(pTarget))
        db.AddButton("worldban", "`4World Ban``");

    db.AddSpacer().EndDialog("popup", "", "Continue");
    pPlayer->SendOnDialogRequest(db.Get());
}

void PopupDialog::RequestSelf(GamePlayer* pPlayer)
{
    if (!pPlayer)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    PlayerProgress& progressData = pPlayer->GetProgressData();
    PlayerLoginDetail& loginDetail = pPlayer->GetLoginDetail();
    CharacterData& characterData = pPlayer->GetCharData();
    PlayerInventory& inventory = pPlayer->GetInventory();
    PlayerPlayModController& modController = pPlayer->GetModController();

    DialogBuilder db;
    db.SetDefaultColor('o');

    TileInfo* pLockAcessTile = pPlayer->GetLockAcessTile();
    if (pLockAcessTile)
    {
        TileExtra_Lock* pTileExtra = pLockAcessTile->GetExtra<TileExtra_Lock>();
        if (!pTileExtra)
        {
            pPlayer->SetLockAccessTile(-1);
        }
        else
        {
            GamePlayer* pOwner = GetPlayerManager()->GetPlayerByNetID(pPlayer->GetLockAcessOwnerID());
            if (!pOwner || pOwner->GetCurrentWorld() != pPlayer->GetCurrentWorld() ||
                pOwner->GetUserID() != pTileExtra->ownerID)
            {
                pPlayer->SetLockAccessTile(-1);
                pLockAcessTile = nullptr;
            }

            if (pLockAcessTile)
            {
                ItemInfo* pLockItem = GetItemInfoManager()->GetItemByID(pLockAcessTile->GetFG());
                if (pLockItem)
                {
                    db.AddButton("acceptlock", "`2Accept Access on " + pLockItem->name);
                }
            }
        }
    }

    db.AddPlayerInfo(pPlayer->GetDisplayName(true), pPlayer->GetPlayerLevel(),
                     progressData.GetProgress(PLAYER_PROGRESS_XP), pPlayer->GetPlayerNextLevelXP())
        .AddSpacer();

    uint8 onlineStatus = progressData.GetProgress(PLAYER_PROGRESS_ONLINE_STATUS) & 3;

    if (loginDetail.protocol > 127)
    {
        db.AddCustomButton("title_edit", "image:interface/large/gui_wrench_title.rttex;image_size:400,260;width:0.19;");

        if (onlineStatus == PLAYER_ONLINE_STATUS_DEFAULT)
            db.AddCustomButton(
                "set_online_status",
                "image:interface/large/gui_wrench_online_status_1green.rttex;image_size:400,260;width:0.19;");
        else if (onlineStatus == PLAYER_ONLINE_STATUS_AWAY)
            db.AddCustomButton(
                "set_online_status",
                "image:interface/large/gui_wrench_online_status_2yellow.rttex;image_size:400,260;width:0.19;");
        else
            db.AddCustomButton(
                "set_online_status",
                "image:interface/large/gui_wrench_online_status_3red.rttex;image_size:400,260;width:0.19;");

        if (pPlayer->HasFlag(PLAYER_FLAG_SUPPORTER) || pPlayer->HasFlag(PLAYER_FLAG_SUPER_SUPPORTER))
            db.AddCustomButton("billboard_edit",
                               "image:interface/large/gui_wrench_edit_billboard.rttex;image_size:400,260;width:0.19");

        db.AddCustomButton("notebook_edit",
                           "image:interface/large/gui_wrench_notebook.rttex;image_size:400,260;width:0.19;");
        db.AddCustomButton("goals",
                           "image:interface/large/gui_wrench_goals_quests.rttex;image_size:400,260;width:0.19;");
        db.AddCustomButton("my_worlds",
                           "image:interface/large/gui_wrench_my_worlds.rttex;image_size:400,260;width:0.19;");
        db.AddCustomButton("alist",
                           "image:interface/large/gui_wrench_achievements.rttex;image_size:400,260;width:0.19;");

        if (inventory.GetClothByPart(BODY_PART_HAND) == ITEM_ID_BATTLE_LEASH &&
            progressData.GetProgress(PLAYER_PROGRESS_PET_2_0) != 0)
            db.AddCustomButton("pets",
                               "image:interface/large/gui_wrench_battle_pets.rttex;image_size:400,260;width:0.19;");
    }
    else
    {
        db.AddButton("title_edit", "`$Title``");

        // convert status to textureX
        if (onlineStatus == PLAYER_ONLINE_STATUS_BUSY)
            onlineStatus = 30;
        else if (onlineStatus = PLAYER_ONLINE_STATUS_AWAY)
            onlineStatus = 29;
        else
            onlineStatus = 28;
        db.AddInnerImageLabelButton("set_online_status", "`$Set Online Status``", "game/tiles_page14.rttex",
                                    onlineStatus, 23);

        if (pPlayer->HasFlag(PLAYER_FLAG_SUPPORTER) || pPlayer->HasFlag(PLAYER_FLAG_SUPER_SUPPORTER))
            db.AddButton("billboard_edit", "`$Edit Billboard``");

        db.AddButton("notebook_edit", "`$Notebook``");
        db.AddButton("goals", "`$Goals & Quests``");
        // check daily bonus

        db.AddButton("my_worlds", "`$My Worlds``");
        db.AddButton("alist", "`$Challanges (" + ToString(progressData.GetCountOfCompletedAchieves()) + "`5/``" +
                                  ToString(ACHIEVEMENT_COUNT) + ")");

        if (inventory.GetClothByPart(BODY_PART_HAND) == ITEM_ID_BATTLE_LEASH &&
            progressData.GetProgress(PLAYER_PROGRESS_PET_2_0) != 0)
            db.AddButton("pets", "`wBattle Pets!``");
    }

    if (modController.GetActiveModCount() > 0)
    {
        db.AddTextBox("`wActive effects:``");
        modController.BuildActiveModsDialog(db);
    }

    db.AddSpacer();
    db.AddTextBox("`oYou have `w" + ToString(inventory.GetInventorySize()) + "`` backpack slots.``");

    Vector2Float vPlayerWorldPos = pPlayer->GetWorldPos();
    int32 posX = vPlayerWorldPos.x / 32;
    int32 posY = vPlayerWorldPos.y / 32;

    string worldInfo = "`oCurrent world: `w" + pWorld->GetWorlName() + "`` (`w" + ToString(posX) + "``, `w" +
                       ToString(posY) + "``) (`w" + ToString(pWorld->GetPlayerCount()) + "`` ";
    if (pWorld->GetPlayerCount() == 1)
        worldInfo += "person)";
    else
        worldInfo += "people)";

    db.AddTextBox(worldInfo);
    db.EndDialog("plyr_wrench", "", "Continue");

    pPlayer->SendOnDialogRequest(db.Get());
}

void PopupDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    auto pButtonClicked = packet.Find("buttonClicked"_hash);
    if (!pButtonClicked)
        return;

    if (pButtonClicked->valueSize == 0 || pButtonClicked->valueSize > 35)
        return;

    std::string_view clickedButton = pButtonClicked->GetStringView();

    if (auto pNetID = packet.Find("netID"_hash))
    {
        uint32 netID = 0;
        if (!pNetID->GetUInt(netID) != TO_INT_SUCCESS)
            return;

        GamePlayer* pTarget = GetPlayerManager()->GetPlayerByNetID(netID);
        if (!pTarget)
            return;

        if (pTarget->GetCurrentWorld() != pWorld->GetInstanceID())
        {
            pPlayer->SendOnTalkBubble("Hmm, that person left.", false);
            return;
        }

        if (clickedButton == "kick")
        {
            return;
        }
        else if (clickedButton == "worldban")
        {
            if (!pPlayer->AbleToWorldBanSomeone(pTarget))
            {
                pPlayer->SendOnTextOverlay("Can't `4world ban``, is no longer in a locked area you control!");
                return;
            }

            pWorld->PlaySFXForEveryone("repair.wav");
            pWorld->SendConsoleMessageToAll(pPlayer->GetDisplayName(true) + "`4 world bans`w" +
                                            pTarget->GetDisplayName(true) + "`` from `w" + pWorld->GetWorlName() +
                                            "``!");

            pPlayer->SendOnConsoleMessage("You've banned " + pPlayer->GetDisplayName(true) + "`` from `w" +
                                          pWorld->GetWorlName() +
                                          "`` for one hour.  You can also type `#/uba`` to unban him/her early.");

            pWorld->GetBannedPlayers().RegisterFailure(pTarget->GetAddressNum(), 1, 3600);
            GetWorldManager()->PlayerJoinRequest(pTarget, "EXIT");
            return;
        }
    }
    else
    {

        if (clickedButton == "acceptlock")
        {
            TileInfo* pLockAcessTile = pPlayer->GetLockAcessTile();
            if (!pLockAcessTile)
                return;

            TileExtra_Lock* pTileExtra = pLockAcessTile->GetExtra<TileExtra_Lock>();
            if (!pTileExtra)
                return;

            ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pLockAcessTile->GetFG());
            if (!pItem)
                return;

            DialogBuilder db;
            db.SetDefaultColor('o')
                .AddLabelWithIcon("Accept Access To World", pItem->id)
                .AddSmallText("When you have access to a world, you are responsible for anything the world's owners or "
                              "other admins do.")
                .AddSmallText("Don't accept access to a world unless it is from people you trust.")
                .AddSmallText(
                    "You can remove your access later by either wrenching the lock, or typing `2/unaccess`` to "
                    "remove yourself from all locks in the world.")
                .AddSpacer()
                .AddSmallText("Are you sure you want to be added to this " + pItem->name)
                .EndDialog("acceptaccess", "Yes", "No");

            pPlayer->SendOnDialogRequest(db.Get());
            return;
        }

        if (clickedButton == "title_edit")
        {
            RequestTitleEdit(pPlayer);
            return;
        }

        if (clickedButton == "billboard_edit")
        {
            if (pPlayer->HasFlag(PLAYER_FLAG_SUPER_SUPPORTER) || pPlayer->HasFlag(PLAYER_FLAG_SUPPORTER))
            {
                RequestBillboardEdit(pPlayer);
            }
            return;
        }
    }
}

void PopupDialog::HandleTitleEdit(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    PlayerProgress& progressData = pPlayer->GetProgressData();

    if (auto pLegendary = packet.Find("checkbox_legendary_title"_hash))
    {
        bool val;
        if (pLegendary->GetBool(val) != TO_INT_SUCCESS)
            return;
        progressData.ModifyTitleActivation(PLAYER_TITLE_LEGEND, val);
    }

    if (auto pDoctor = packet.Find("checkbox_doctor_title"_hash))
    {
        bool val;
        if (pDoctor->GetBool(val) != TO_INT_SUCCESS)
            return;
        progressData.ModifyTitleActivation(PLAYER_TITLE_DOCTOR, val);
    }

    if (auto pMaxLvl = packet.Find("checkbox_max_level_title"_hash))
    {
        bool val;
        if (pMaxLvl->GetBool(val) != TO_INT_SUCCESS)
            return;
        progressData.ModifyTitleActivation(PLAYER_TITLE_MAX_LVL, val);
    }

    if (auto pMaster = packet.Find("checkbox_master_title"_hash))
    {
        bool val;
        if (pMaster->GetBool(val) != TO_INT_SUCCESS)
            return;
        progressData.ModifyTitleActivation(PLAYER_TITLE_MASTER, val);
    }

    if (auto pG4g = packet.Find("checkbox_g4g_title"_hash))
    {
        bool val;
        if (pG4g->GetBool(val) != TO_INT_SUCCESS)
            return;
        progressData.ModifyTitleActivation(PLAYER_TITLE_G4G, val);
    }

    pWorld->SendNameChangeToAll(pPlayer);
    pWorld->SendOnCountryStateToAll(pPlayer);
}

void PopupDialog::HandleAcceptAccess(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer || pPlayer->GetCurrentWorld() == 0)
        return;

    pPlayer->AcceptLockAccess();
}

void PopupDialog::HandleBillboardEdit(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer || pPlayer->GetCurrentWorld() == 0)
        return;

    if (!pPlayer->HasFlag(PLAYER_FLAG_SUPER_SUPPORTER) && !pPlayer->HasFlag(PLAYER_FLAG_SUPPORTER))
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    string errMsg;
    bool updatedItem = false;
    bool shouldRefresh = false;

    PlayerExtraData& extraData = pPlayer->GetExtraData();

    if (auto pBillboardItem = packet.Find("billboard_item"_hash))
    {
        int32 itemID;
        if (pBillboardItem->GetInt(itemID) != TO_INT_SUCCESS)
            return;

        ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
        if (!pItem)
            return;

        updatedItem = true;

        if (pPlayer->GetInventory().GetCountOfItem(pItem->id) == 0)
        {
            errMsg += "Cannot display billboard. The item is missing from your inventory.\n";
        }
        else if (pItem->HasFlag(ITEM_FLAG_UNTRADEABLE) || pItem->HasFlag(ITEM_FLAG_MOD))
        {
            errMsg += "Untradeable items can't be advertised on billboards.\n";
        }
        else
        {
            extraData.Set(PLAYER_EXTRA_BILLBOARD_ITEM_ID, itemID);
            shouldRefresh = true;
        }
    }

    if (auto pBillboardToggle = packet.Find("billboard_toggle"_hash))
    {
        bool val;
        if (pBillboardToggle->GetBool(val) != TO_INT_SUCCESS)
            return;

        extraData.Set(PLAYER_EXTRA_BILLBOARD_SHOW, val);
    }

    if (auto pBillboardBuyingToggle = packet.Find("billboard_buying_toggle"_hash))
    {
        bool val;
        if (pBillboardBuyingToggle->GetBool(val) != TO_INT_SUCCESS)
            return;

        extraData.Set(PLAYER_EXTRA_BILLBOARD_IS_BUY, val);
    }

    if (auto pSetPrice = packet.Find("setprice"_hash))
    {
        int32 price;
        if (pSetPrice->GetInt(price) != TO_INT_SUCCESS)
            return;

        if (price < 0)
            errMsg += "Price can't be negative. That's beyond science\n";
        else if (price == 0)
            errMsg += "Price can't be zero. That's free!\n";
        else
        {
            extraData.Set(PLAYER_EXTRA_BILLBOARD_PRICE, price);
        }
    }

    auto pChkPerItem = packet.Find("chk_peritem"_hash);
    auto pChkPerLock = packet.Find("chk_perlock"_hash);
    if (pChkPerItem && pChkPerLock)
    {
        bool isPerItem;
        if (pChkPerItem->GetBool(isPerItem) != TO_INT_SUCCESS)
            return;

        bool isPerLock;
        if (pChkPerLock->GetBool(isPerLock) != TO_INT_SUCCESS)
            return;

        if (isPerItem && isPerLock)
            errMsg += "You can't select both 'locks per item' and 'items per lock'.\n";
        else if (!isPerItem && !isPerLock)
            errMsg += "You need to pick one pricing method - 'locks per item' or 'items per lock'.\n";
        else
            extraData.Set(PLAYER_EXTRA_BILLBOARD_IS_LOCK_PER, isPerItem);
    }

    if (errMsg.empty())
    {
        if (!updatedItem)
        {
            if (extraData.Get(PLAYER_EXTRA_BILLBOARD_SHOW).GetBool())
            {
                if (pPlayer->GetInventory().GetCountOfItem(extraData.Get(PLAYER_EXTRA_BILLBOARD_ITEM_ID).GetINT()) == 0)
                {
                    extraData.Set(PLAYER_EXTRA_BILLBOARD_SHOW, false);
                    pPlayer->SendOnTalkBubble("Cannot display billboard. The item is missing from your inventory.",
                                              false);
                    shouldRefresh = false;
                }
            }

            pWorld->SendOnBillboardChangeToAll(pPlayer);
        }
    }
    else
    {
        pPlayer->SendOnTalkBubble(errMsg, false);
        shouldRefresh = false;
    }

    if (shouldRefresh)
        RequestBillboardEdit(pPlayer);
}

void PopupDialog::RequestTitleEdit(GamePlayer* pPlayer)
{
    if (!pPlayer || pPlayer->GetCurrentWorld() == 0)
        return;

    DialogBuilder db;
    db.SetDefaultColor('o').AddLabel("Select Title:", true);

    PlayerProgress& progressData = pPlayer->GetProgressData();
    if (progressData.GetProgress(PLAYER_PROGRESS_TITLES) == 0)
    {
        db.AddLabel("No Titles Obtained", true);
    }
    else
    {
        if (progressData.HasTitle(PLAYER_TITLE_LEGEND))
            db.AddCheckBox("checkbox_legendary_title", "' of Legend'", progressData.IsTitleActive(PLAYER_TITLE_LEGEND));

        if (progressData.HasTitle(PLAYER_TITLE_DOCTOR))
            db.AddCheckBox("checkbox_doctor_title", "'Dr.'", progressData.IsTitleActive(PLAYER_TITLE_DOCTOR));

        if (progressData.HasTitle(PLAYER_TITLE_MAX_LVL))
            db.AddCheckBox("checkbox_max_level_title", "Level 125", progressData.IsTitleActive(PLAYER_TITLE_MAX_LVL));

        if (progressData.HasTitle(PLAYER_TITLE_MASTER))
            db.AddCheckBox("checkbox_master_title", "Master", progressData.IsTitleActive(PLAYER_TITLE_MASTER));

        if (progressData.HasTitle(PLAYER_TITLE_G4G))
            db.AddCheckBox("checkbox_g4g_title", "Grow4Good Title", progressData.IsTitleActive(PLAYER_TITLE_G4G));
    }

    db.AddSpacer().AddButton("", "OK").EndDialog("title_edit", "", "");

    pPlayer->SendOnDialogRequest(db.Get());
}

void PopupDialog::RequestBillboardEdit(GamePlayer* pPlayer)
{
    if (!pPlayer || pPlayer->GetCurrentWorld() == 0)
        return;

    PlayerExtraData& extraData = pPlayer->GetExtraData();
    int32 itemID = extraData.Get(PLAYER_EXTRA_BILLBOARD_ITEM_ID).GetINT();
    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
        return;

    DialogBuilder db;
    db.SetDefaultColor('o').AddLabelWithIcon("`wTrade Billboard``", ITEM_ID_DUMB_BILLBOARD, true).AddSpacer();

    if (pItem->id != ITEM_ID_BLANK)
    {
        if (pPlayer->GetInventory().GetCountOfItem(pItem->id) == 0)
        {
            db.AddLabelWithIcon("`4Selected item doesn't exist in your inventory.``", pItem->id);
        }
        else if (!(pItem->HasFlag(ITEM_FLAG_UNTRADEABLE) || pItem->HasFlag(ITEM_FLAG_MOD)))
        {
            db.AddLabelWithIcon(pItem->name + "``", pItem->id);
        }
    }

    db.AddItemPicker("billboard_item", "`wSelect Billboard Item``", "Choose an item to put on your billboard!")
        .AddSpacer()
        .AddCheckBox("billboard_toggle", "`$Show Billboard", extraData.Get(PLAYER_EXTRA_BILLBOARD_SHOW).GetBool());

    if (pPlayer->GetLoginDetail().protocol >= 167)
    {
        db.AddCheckBox("billboard_buying_toggle", "`$Is Buying",
                       extraData.Get(PLAYER_EXTRA_BILLBOARD_IS_BUY).GetBool());
    }

    db.AddTextInput("setprice", "Price of item:", ToString(extraData.Get(PLAYER_EXTRA_BILLBOARD_PRICE).GetINT()), 5)
        .AddCheckBox("chk_peritem", "World Locks per Item", extraData.Get(PLAYER_EXTRA_BILLBOARD_IS_LOCK_PER).GetBool())
        .AddCheckBox("chk_perlock", "Items per World Lock",
                     !extraData.Get(PLAYER_EXTRA_BILLBOARD_IS_LOCK_PER).GetBool())
        .AddSpacer()
        .EndDialog("billboard_edit", "Update", "Close");

    pPlayer->SendOnDialogRequest(db.Get());
}

void RegisterDialog::Request(GamePlayer* pPlayer, const string& namePlaceholder, const string& passPlaceholder,
                             const string& passVerifPlaceholder, const string& errorMsg)
{
    if (!pPlayer || pPlayer->HasGrowID())
    {
        return;
    }

    DialogBuilder db;
    db.SetDefaultColor('o').AddLabelWithIcon("`wGet a GrowID", ITEM_ID_FIST, true).AddSpacer();

    if (!errorMsg.empty())
    {
        db.AddTextBox(errorMsg).AddSpacer();
    }

    db.AddTextBox("By choosing a `wGrowID``, you can use a name and password to logon from any device. Your `wname`` "
                  "will be shown to other players!")
        .AddTextInput("logon", "Name", namePlaceholder, 18)
        .AddSpacer()
        .AddTextInputPassword("password", "Password", passPlaceholder, 18)
        .AddTextInputPassword("verify_password", "Verify Password", passVerifPlaceholder, 18)
        .AddSpacer()
        .EndDialog("growid_apply", "Get My GrowID!", "Cancel");

    pPlayer->SendOnDialogRequest(db.Get(), 1000);
}

void RegisterDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer || pPlayer->HasGrowID())
        return;

    auto pName = packet.Find("logon"_hash);
    auto pPass = packet.Find("password"_hash);
    auto pVerifPass = packet.Find("verify_password"_hash);

    if (!pName || !pPass || !pVerifPass)
        return;

    string name = pName->GetString();
    string pass = pPass->GetString();
    string verifPass = pVerifPass->GetString();
    string error = "";

    if (name.find(" ") != string::npos)
        error = "`4Oops!``  Your `$GrowID`` name can't have spaces in it.";
    else if (name.find("`") != string::npos)
        error = "`4Oops!`` You can't use color codes in your `$GrowID``.";
    else if (pass != verifPass)
        error = "`4Oops!``  Passwords don't match.  Try again.";
    else if (pass.size() < 3 || pass.size() > 18)
        error = "`4Oops!``  Your password must be between `$3`` and `$18`` characters long.";
    else if (name.size() < 3 || name.size() > 12)
        error = "`4Oops!``  Your `wGrowID`` must be between `$3`` and `$12`` characters long.";

    if (!error.empty())
    {
        Request(pPlayer, name, pass, verifPass, error);
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
        .AddLabelWithIcon("`wGrowID GET!", ITEM_ID_FIST, true)
        .AddTextBox("A `wGrowID`` with the log on of `w" + growID + "`` and the password of `w" + pass +
                    "`` created. Write them down, they will be required to log on from now on!")
        .EndDialog("growid_succ", "", "Continue");

    pPlayer->SendSetHasGrowID(true);

    pPlayer->SendOnDialogRequest(db.Get());
    pPlayer->PlaySFX("piano_nice.wav");

    if (pPlayer->GetCurrentWorld() != 0)
    {
        World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
        if (pWorld)
        {
            pWorld->SendNameChangeToAll(pPlayer);
        }
    }
}

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

void SignDialog::Request(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile)
        return;

    TileExtra_Sign* pTileExtra = pTile->GetExtra<TileExtra_Sign>();
    if (!pTileExtra)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        return;

    DialogBuilder db;
    db.SetDefaultColor('o').AddLabelWithIcon("`wEdit " + pItem->name, pItem->id, true);

    if (pItem->type == ITEM_TYPE_CHAL_FLAG)
    {
        db.AddTextBox("Enter an ID. This flag will be connected to the Challenge Timer with the same ID.``");
    }
    else if (IsPathMarker(pItem->id))
    {
        db.AddTextBox("Enter an ID. You can use this as a destination for Doors.``");
    }
    else
    {
        db.AddTextBox("What would you like to write on this sign?``");
    }

    db.AddTextInput("sign_text", "", pTileExtra->text, 128)
        .EmbedData("tilex", pTile->GetPos().x)
        .EmbedData("tiley", pTile->GetPos().y)
        .EndDialog("sign_edit", "OK", "Cancel");

    pPlayer->SendOnDialogRequest(db.Get());
}

void SignDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pTileX = packet.Find("tilex"_hash);
    if (!pTileX)
        return;

    auto pTileY = packet.Find("tiley"_hash);
    if (!pTileY)
        return;

    auto pSignText = packet.Find("sign_text"_hash);
    if (!pSignText)
        return;

    if (pSignText->valueSize > 128)
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

    TileExtra_Sign* pTileExtra = pTile->GetExtra<TileExtra_Sign>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The sign is gone!", false);
        return;
    }

    if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem)
        return;

    if (pItem->type == ITEM_TYPE_RACE_FLAG)
    {
        if (pSignText->valueSize > 11)
        {
            pPlayer->SendOnTalkBubble("That ID is too long!", false);
            return;
        }

        string raceID = ToUpper(pSignText->GetString());
        RemoveGTColorCodes(raceID);
        pTileExtra->text = raceID;
    }
    else if (IsPathMarker(pItem->id))
    {
        if (pSignText->valueSize > 11)
        {
            pPlayer->SendOnTalkBubble("That door ID is too long!", false);
            return;
        }

        string doorID = ToUpper(pSignText->GetString());
        RemoveGTColorCodes(doorID);
        pTileExtra->text = doorID;
    }
    else
    {
        if (pSignText->GetStringView().find("__&%@PL@%&__") != string::npos)
        {
            pPlayer->SendOnTalkBubble("You try to write the magic symbols down but they disappear!", false);
            return;
        }

        string text = pSignText->GetString();
        RemoveGTColorCodes(text);

        pTileExtra->text = text;
        pWorld->SendTileUpdate(tileX, tileY);
    }
}

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

void SuckerBlockDialog::Request(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile)
        return;

    TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>();
    if (!pTileExtra)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem || pItem->type != ITEM_TYPE_SUCKER)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    pWorld->GetSuckerBlockManager().IsCorrupted(pTile);

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon(pItem->name + "``", pItem->id, true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y);

    bool isOwner = pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID());

    if (pTileExtra->itemID == ITEM_ID_BLANK)
    {
        if (isOwner)
        {
            db.AddTextBox("`6The machine is empty.``")
                .AddItemPicker("selectitem", "`wChoose Item``", "Choose an item to put in the " + pItem->name);
        }
        else
        {
            db.AddTextBox("There is no item selected.");
        }

        db.EndDialog("itemsucker", "", "Close");
        pPlayer->SendOnDialogRequest(db.Get());
        return;
    }

    ItemInfo* pSuckedItemInfo = GetItemInfoManager()->GetItemByID(pTileExtra->itemID);
    if (!pSuckedItemInfo)
        return;

    int32 tileFg = pTile->GetFG();

    db.AddSpacer().AddLabelWithIcon("`2" + pSuckedItemInfo->name + "``", pSuckedItemInfo->id);

    if (isOwner)
    {
        if (pTileExtra->count == 0)
        {
            db.AddTextBox("`6The machine is currently empty!``");
        }
        else
        {
            db.AddTextBox("Machine contains " + ToString(pTileExtra->count) + " `6" + pSuckedItemInfo->name + "``");
        }
    }

    bool canPlantOrBuildItem = SuckerBlockManager::IsAllowedToBuildOrPlantItem(pTileExtra->itemID);

    if (isOwner)
    {
        if (SuckerBlockManager::GetMachineCapacity(pItem->id) == pTileExtra->count)
        {
            db.AddTextBox("`4The machine is at maximum capacity!``");
        }
        else
        {
            if (pPlayer->GetInventory().GetCountOfItem(pTileExtra->itemID) > 0)
                db.AddButton("additem", "Add Items to the machine");
        }

        if (pTileExtra->count == 0)
        {
            db.AddButton("clearitem", "Change Item");
        }
        else if (pTileExtra->count == 1)
        {
            db.AddButton("retrieveitem", "Retrieve Item");
        }
        else
        {
            if (pPlayer->GetInventory().GetCountOfItem(pTileExtra->itemID) >= 200)
                db.AddTextBox("You have too many " + pSuckedItemInfo->name + "!");
            else
                db.AddButton("retrieveitem", "Retrieve Items");
        }
    }

    if (tileFg != ITEM_ID_GAIAS_BEACON && tileFg != ITEM_ID_UNSTABLE_TESSERACT)
    {
        if (canPlantOrBuildItem)
        {
            if (pTileExtra->isPlanting == 1)
            {
                if (pSuckedItemInfo->type == ITEM_TYPE_SEED)
                    db.AddTextBox("Planting mode: `5ACTIVE``");
                else
                    db.AddTextBox("Building mode: `5ACTIVE``");
            }
            else
            {
                if (pSuckedItemInfo->type == ITEM_TYPE_SEED)
                {
                    db.AddTextBox("Planting mode: `6DISABLED``");
                    if (isOwner)
                        db.AddTextBox("Punch to activate planting mode.");
                }
                else
                {
                    db.AddTextBox("Building mode: `6DISABLED``");
                    if (isOwner)
                        db.AddTextBox("Punch to activate building mode.");
                }
            }

            if (pPlayer->GetInventory().GetCountOfItem(ITEM_ID_MAGPLANT_5000_REMOTE) < 1)
                db.AddButton("getplantationdevice", "Get Remote");
        }
        else
        {
            db.AddTextBox("`6You cannot place this item.``");
        }
    }

    if (isOwner)
    {
        db.AddCheckBox("chk_enablesucking", "Enable Collection", pTileExtra->isSucking == 1)
            .EndDialog("itemsucker", "Update", "Close");
    }
    else
    {
        if (tileFg != ITEM_ID_GAIAS_BEACON && tileFg != ITEM_ID_UNSTABLE_TESSERACT)
        {
            db.EndDialog("itemsucker", "", "Close");
        }
        else
            return;
    }

    pPlayer->SendOnDialogRequest(db.Get());
}

void SuckerBlockDialog::RequestAddItem(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile)
        return;

    TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>();
    if (!pTileExtra)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem || pItem->type != ITEM_TYPE_SUCKER)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    if (!pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
        return;

    int32 maxCapacity = SuckerBlockManager::GetMachineCapacity(pItem->id);

    if (pTileExtra->count >= maxCapacity)
    {
        pPlayer->SendOnTalkBubble(
            "You cannot add more items, the " + pItem->name + " is currently at maximum capacity.", false);
        return;
    }

    ItemInfo* pSuckedItemInfo = GetItemInfoManager()->GetItemByID(pTileExtra->itemID);
    if (!pSuckedItemInfo)
        return;

    int32 playerItemCount = pPlayer->GetInventory().GetCountOfItem(pTileExtra->itemID);
    if (playerItemCount < 1)
    {
        pPlayer->SendOnTalkBubble("You don't have " + pSuckedItemInfo->name + "!", false);
        return;
    }

    int32 availableSpace = maxCapacity - pTileExtra->count;
    int32 defaultAmountToAdd = Min(playerItemCount, availableSpace);

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddSpacer()
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y)
        .AddLabelWithIcon("`2" + pSuckedItemInfo->name + "``", pSuckedItemInfo->id)
        .AddTextBox("You have " + ToString(playerItemCount) + " `2" + pSuckedItemInfo->name + "`` in your backpack.")
        .AddTextBox("How many `2" + pSuckedItemInfo->name + "`` would you like to add?")
        .AddTextInput("itemtoadd", "Amount:", ToString(defaultAmountToAdd), 20)
        .EndDialog("itemaddedtosucker", "Add", "Close");

    pPlayer->SendOnDialogRequest(db.Get());
}

void SuckerBlockDialog::RequestRetrieveItem(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile)
        return;

    TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>();
    if (!pTileExtra)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem || pItem->type != ITEM_TYPE_SUCKER)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    if (!pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
        return;

    if (pTileExtra->count < 1)
    {
        pPlayer->SendOnTalkBubble("You don't have any items to retrieve.", false);
        return;
    }

    ItemInfo* pSuckedItemInfo = GetItemInfoManager()->GetItemByID(pTileExtra->itemID);
    if (!pSuckedItemInfo)
        return;

    int32 playerHoldCount = pPlayer->GetInventory().GetCountOfItem(pSuckedItemInfo->id);
    int32 remainingSpace = pSuckedItemInfo->maxCanHold - playerHoldCount;

    if (remainingSpace <= 0 || !pPlayer->GetInventory().HaveRoomForItem(pSuckedItemInfo->id, 1))
    {
        pPlayer->SendOnTalkBubble("You don't have any space left in your backpack for this.", false);
        return;
    }

    int32 defaultRetrieveAmount = Min(pTileExtra->count, remainingSpace);

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddSpacer()
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y)
        .AddLabelWithIcon("`2" + pSuckedItemInfo->name + "``", pSuckedItemInfo->id)
        .AddTextBox("How many `2" + pSuckedItemInfo->name + "`` would you like to remove?")
        .AddTextInput("itemtoremove", "Amount:", ToString(defaultRetrieveAmount), 20)
        .EndDialog("itemremovedfromsucker", "Retrieve", "Close");

    pPlayer->SendOnDialogRequest(db.Get());
}

void SuckerBlockDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pTileX = packet.Find("tilex"_hash);
    auto pTileY = packet.Find("tiley"_hash);
    if (!pTileX || !pTileY)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    int32 tileX = 0, tileY = 0;
    if (pTileX->GetInt(tileX) != TO_INT_SUCCESS || pTileY->GetInt(tileY) != TO_INT_SUCCESS)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if (!pTile)
        return;

    TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? It's gone!", false);
        return;
    }

    SuckerBlockManager& suckerMgr = pWorld->GetSuckerBlockManager();
    bool tileNeedsUpdate = false;
    auto pButtonClicked = packet.Find("buttonClicked"_hash);

    int32 tileFG = pTile->GetFG();

    if (pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
    {
        if (auto pEnableSucking = packet.Find("chk_enablesucking"_hash))
        {
            bool val = false;
            if (pEnableSucking->GetBool(val) == TO_INT_SUCCESS)
            {
                uint8 newSuckingState = val ? 1 : 0;
                if (pTileExtra->isSucking != newSuckingState)
                {
                    pTileExtra->isSucking = newSuckingState;
                    tileNeedsUpdate = true;
                }
            }
        }

        if (auto pItemID = packet.Find("selectitem"_hash))
        {
            uint32 itemID = 0;
            if (pItemID->GetUInt(itemID) == TO_INT_SUCCESS)
            {
                ItemInfo* pSelectedItem = GetItemInfoManager()->GetItemByID(itemID);
                if (pSelectedItem)
                {
                    if (pPlayer->GetInventory().GetCountOfItem(pSelectedItem->id) < 1)
                    {
                        pPlayer->SendOnTalkBubble("You don't have that.", false);
                        return;
                    }

                    suckerMgr.IsCorrupted(pTile);
                    if (pTileExtra->itemID == ITEM_ID_BLANK)
                    {
                        if (SuckerBlockManager::IsAllowedItemInMachine(pSelectedItem->id, tileFG))
                        {
                            suckerMgr.ChangeSuckerItem(pTile, pTileExtra->itemID, pSelectedItem->id);

                            pTileExtra->itemID = pSelectedItem->id;
                            pTileExtra->count = 0;
                            pTileExtra->isSucking = 1;
                            pTileExtra->isPlanting = 0;

                            tileNeedsUpdate = true;
                        }
                        else
                        {
                            if (tileFG == ITEM_ID_GAIAS_BEACON && pSelectedItem->type != ITEM_TYPE_SEED)
                            {
                                pPlayer->SendOnTalkBubble("You can only store seeds in this machine.", false);
                                return;
                            }

                            if (tileFG == ITEM_ID_UNSTABLE_TESSERACT && pSelectedItem->type != ITEM_TYPE_SEED)
                            {
                                pPlayer->SendOnTalkBubble("You cannot store seeds in this machine.", false);
                                return;
                            }

                            pPlayer->SendOnTalkBubble("This item is not compatible.", false);
                            return;
                        }
                    }
                    else
                    {
                        pPlayer->SendOnTalkBubble("You cannot select an item.", false);
                    }
                }
            }
        }

        if (pButtonClicked)
        {
            std::string_view buttonClicked = pButtonClicked->GetStringView();

            if (buttonClicked == "clearitem")
            {
                if (pTileExtra->count != 0)
                {
                    pPlayer->SendOnTalkBubble("Empty the machine first.", false);
                    return;
                }

                suckerMgr.ToggleRemote(pPlayer, pTile);
                suckerMgr.ChangeSuckerItem(pTile, pTileExtra->itemID, ITEM_ID_BLANK);

                pTileExtra->itemID = 0;
                pTileExtra->count = 0;
                pTileExtra->isSucking = 1;
                pTileExtra->isPlanting = 0;

                pWorld->SendTileUpdate(pTile);
                Request(pPlayer, pTile);
                return;
            }

            if (buttonClicked == "additem")
            {
                RequestAddItem(pPlayer, pTile);
                return;
            }

            if (buttonClicked == "retrieveitem")
            {
                RequestRetrieveItem(pPlayer, pTile);
                return;
            }
        }
    }

    if (tileFG != ITEM_ID_GAIAS_BEACON && tileFG != ITEM_ID_UNSTABLE_TESSERACT)
    {
        if (pButtonClicked && pButtonClicked->GetStringView() == "getplantationdevice")
        {
            suckerMgr.GiveRemoteToPlayer(pPlayer);
            return;
        }
    }

    if (tileNeedsUpdate)
    {
        pWorld->SendTileUpdate(pTile);
    }
}

void SuckerBlockDialog::HandleAddItem(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pTileX = packet.Find("tilex"_hash);
    auto pTileY = packet.Find("tiley"_hash);
    if (!pTileX || !pTileY)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    int32 tileX = 0, tileY = 0;
    if (pTileX->GetInt(tileX) != TO_INT_SUCCESS || pTileY->GetInt(tileY) != TO_INT_SUCCESS)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if (!pTile)
        return;

    TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? It's gone!", false);
        return;
    }

    if (!pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
        return;

    ItemInfo* pSuckedItem = GetItemInfoManager()->GetItemByID(pTileExtra->itemID);
    if (!pSuckedItem || pSuckedItem->HasFlag(ITEM_FLAG_UNTRADEABLE))
        return;

    int32 itemToAdd = 0;
    auto pItemToAdd = packet.Find("itemtoadd"_hash);
    if (!pItemToAdd || pItemToAdd->GetInt(itemToAdd) != TO_INT_SUCCESS || itemToAdd <= 0)
        return;

    int32 playerInventoryCount = pPlayer->GetInventory().GetCountOfItem(pSuckedItem->id);
    if (playerInventoryCount < itemToAdd)
    {
        pPlayer->SendOnTalkBubble("You don't have " + ToString(itemToAdd) + " " + pSuckedItem->name + "!", false);
        return;
    }

    if (pTileExtra->count + itemToAdd > SuckerBlockManager::GetMachineCapacity(pTile->GetFG()))
    {
        if (ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG()))
        {
            pPlayer->SendOnTalkBubble("You cannot add this amount, it will overload the " + pItem->name + "!", false);
        }
        return;
    }

    pTileExtra->count += itemToAdd;
    pPlayer->ModifyInventoryItem(pSuckedItem->id, -itemToAdd);
    pWorld->SendTileUpdate(pTile);
}

void SuckerBlockDialog::HandleRetrieveItem(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pTileX = packet.Find("tilex"_hash);
    auto pTileY = packet.Find("tiley"_hash);
    if (!pTileX || !pTileY)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    int32 tileX = 0, tileY = 0;
    if (pTileX->GetInt(tileX) != TO_INT_SUCCESS || pTileY->GetInt(tileY) != TO_INT_SUCCESS)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if (!pTile)
        return;

    TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? It's gone!", false);
        return;
    }

    if (!pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
        return;

    ItemInfo* pSuckedItem = GetItemInfoManager()->GetItemByID(pTileExtra->itemID);
    if (!pSuckedItem || pSuckedItem->HasFlag(ITEM_FLAG_UNTRADEABLE))
        return;

    int32 itemToRemove = 0;
    auto pItemToRemove = packet.Find("itemtoremove"_hash);
    if (!pItemToRemove || pItemToRemove->GetInt(itemToRemove) != TO_INT_SUCCESS || itemToRemove <= 0)
        return;

    if (pTileExtra->count < itemToRemove)
    {
        pPlayer->SendOnTalkBubble("You don't have that many items in the machine.", false);
        return;
    }

    if (!pPlayer->GetInventory().HaveRoomForItem(pSuckedItem->id, itemToRemove))
    {
        pPlayer->SendOnTalkBubble("You don't have enough space in your backpack for this.", false);
        return;
    }

    pTileExtra->count -= itemToRemove;
    pPlayer->ModifyInventoryItem(pSuckedItem->id, itemToRemove);
    pWorld->SendTileUpdate(pTile);
}

void TradeDialog::Request(GamePlayer* pPlayer, InventoryItemInfo* pInvItem)
{
    if (!pPlayer || !pInvItem)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pInvItem->id);
    if (!pItem)
        return;

    if (pPlayer->GetCurrentWorld() == 0)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon("`2Trade`w" + pItem->name, pItem->id, true)
        .AddTextBox("`2Trade how many?``")
        .AddTextInput("count", "", "", 5)
        .EmbedData("itemID", ToItemClientID(pItem->id))
        .EndDialog("trade_item", "OK", "Cancel");

    pPlayer->SendOnDialogRequest(db.Get());
}

void TradeDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pItemID = packet.Find("itemID"_hash);
    if (!pItemID)
        return;

    auto pCount = packet.Find("count"_hash);
    if (!pCount)
        return;

    if (pPlayer->GetCurrentWorld() == 0)
        return;

    int32 itemID = 0;
    if (pItemID->GetInt(itemID) != TO_INT_SUCCESS)
        return;

    int32 count = 0;
    if (pCount->GetInt(count) != TO_INT_SUCCESS)
        return;

    if (count <= 0)
        return;

    pPlayer->GetTradeManager().OnTradeItem(itemID, count, false);
}

void TrashDialog::Request(GamePlayer* pPlayer, int16 itemID)
{
    if (!pPlayer || itemID == ITEM_ID_BLANK)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
    {
        LOGGER_LOG_ERROR("Player %d tried to trash non exist item?", pPlayer->GetUserID());
        return;
    }

    if (pItem->IsUnlimited())
    {
        pPlayer->PlaySFX("cant_place_tile.wav");
        pPlayer->SendOnTextOverlay("You'd be sorry to lose that");
        return;
    }

    uint8 invItemCount = pPlayer->GetInventory().GetCountOfItem(pItem->id);
    if (invItemCount == 0)
        return;

    DialogBuilder db;
    db.SetDefaultColor('o');
    if (pPlayer->HasFlag(PLAYER_FLAG_SUPPORTER) || pPlayer->HasFlag(PLAYER_FLAG_SUPER_SUPPORTER))
    {
        db.AddLabelWithIcon("`4Recycle`` " + pItem->name, pItem->id, true);
    }
    else
    {
        db.AddLabelWithIcon("`4Trash`` " + pItem->name, pItem->id, true);
    }

    db.AddTextBox("How many to `4destory``? (you have " + ToString(invItemCount) + ")")
        .AddTextInput("count", "", "0", 4)
        .EmbedData("itemID", ToItemClientID(pItem->id))
        .EndDialog("trash_item", "OK", "Cancel");

    pPlayer->SendOnDialogRequest(db.Get());
}

void TrashDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pItemID = packet.Find("itemID"_hash);
    auto pCount = packet.Find("count"_hash);

    if (!pItemID || !pCount)
        return;

    uint32 itemID = 0;
    if (pItemID->GetUInt(itemID) != TO_INT_SUCCESS)
        return;

    int32 count = 0;
    if (pCount->GetInt(count) != TO_INT_SUCCESS)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
        return;

    if (count <= 0)
        return;

    if (pItem->HasFlag(ITEM_FLAG_UNTRADEABLE))
    {
        if (count > pPlayer->GetInventory().GetCountOfItem(pItem->id))
            return;

        DialogBuilder db;
        db.SetDefaultColor('o')
            .AddLabelWithIcon("`4Trash ``" + ToString(count) + " " + pItem->name, pItem->id, true)
            .AddTextBox("You are recycling an `9UNTRADABLE`` item. Are you absolutely sure you want to do this? There "
                        "is no way to get the item back if you click yes.")
            .EmbedData("itemID", ToItemClientID(pItem->id))
            .EmbedData("count", count)
            .EndDialog("trash_item2", "Yes, I am sure", "NO!");

        pPlayer->SendOnDialogRequest(db.Get());
        return;
    }

    pPlayer->TrashItem(pItem->id, count);
}

void TrashDialog::HandleUntradeable(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pItemID = packet.Find("itemID"_hash);
    auto pCount = packet.Find("count"_hash);

    if (!pItemID || !pCount)
        return;

    uint32 itemID = 0;
    if (pItemID->GetUInt(itemID) != TO_INT_SUCCESS)
        return;

    int32 count = 0;
    if (pCount->GetInt(count) != TO_INT_SUCCESS)
        return;

    if (count <= 0)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(itemID);
    if (!pItem)
        return;

    if (!pItem->HasFlag(ITEM_FLAG_UNTRADEABLE))
        return;

    pPlayer->TrashItem(itemID, count);
}

void VendingMachineDialog::Request(GamePlayer* pPlayer, TileInfo* pTile)
{
    if (!pPlayer || !pTile)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    TileExtra_VendingMachine* pTileExtra = pTile->GetExtra<TileExtra_VendingMachine>();
    if (!pTileExtra)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem || pItem->type != ITEM_TYPE_VENDING)
        return;

    TileInfo* pPlayerCenterTile = pPlayer->GetTilePlayerOnCenter();
    if (!pPlayerCenterTile)
        return;

    if (pPlayerCenterTile != pTile)
    {
        pPlayer->SendOnTalkBubble("Get closer!", false);
        return;
    }

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon("`w" + pItem->name + "``", pItem->id, true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y);

    if (pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
    {
        if (pTileExtra->itemID == ITEM_ID_BLANK || pTileExtra->stock == 0)
        {
            db.AddTextBox("This machine is empty")
                .AddItemPicker("stockitem", "`wPut an item in``", "Choose an item to put in the machine!");
        }
        else
        {
        }
    }
    else
    {
    }
}

void VendingMachineDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet) {}

void WeatherSpecialDialog::Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem)
{
    if (!pPlayer || !pTile || !pItem)
        return;

    TileExtra_WeatherSpecial* pTileExtra = pTile->GetExtra<TileExtra_WeatherSpecial>();
    if (!pTileExtra)
        return;

    if (pItem->type != ITEM_TYPE_WEATHER_SPECIAL && pItem->type != ITEM_TYPE_WEATHER_SPECIAL2)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        return;

    DialogBuilder db;
    db.SetDefaultColor('o').AddLabelWithIcon(pItem->name + "``", pItem->id, true);

    if (pItem->type == ITEM_TYPE_WEATHER_SPECIAL)
    {
        switch (pItem->id)
        {
            case ITEM_ID_WEATHER_MACHINE_HEATWAVE:
            {
                db.AddTextBox("Adjust the color of your heat wave here, by including 0-255 of Red, Green, and Blue")
                    .AddTextInput("red", "Red:", ToString(TO_COLOR_RED(pTileExtra->color)), 3)
                    .AddTextInput("green", "Green:", ToString(TO_COLOR_GREEN(pTileExtra->color)), 3)
                    .AddTextInput("blue", "Blue:", ToString(TO_COLOR_BLUE(pTileExtra->color)), 3);
                break;
            }

            case ITEM_ID_WEATHER_MACHINE_BACKGROUND:
            {
                int32 bgItemID = pTileExtra->itemID;
                ItemInfo* pBgItem = GetItemInfoManager()->GetItemByID(bgItemID);

                if (!pBgItem || bgItemID == 0 || !pBgItem->IsBackground())
                {
                    bgItemID = ITEM_ID_CAVE_BACKGROUND;
                    pBgItem = GetItemInfoManager()->GetItemByID(bgItemID);
                }

                if (!pBgItem)
                    return;

                db.AddTextBox("You can scan any Background Block to set it up in your weather machine.")
                    .AddTextBox("Current Background: " + pBgItem->name);
                break;
            }

            default:
                break;
        }
    }
    else if (pItem->type == ITEM_TYPE_WEATHER_SPECIAL2)
    {
        switch (pItem->id)
        {
            case ITEM_ID_EPOCH_MACHINE:
            {
                db.AddTextBox("Select your doom:")
                    .AddCheckBox("iceage", "Ice Age", pTileExtra->HasFlag(TILE_EXTRA_EPOCH_ICE_AGE))
                    .AddCheckBox("volcano", "Volcano", pTileExtra->HasFlag(TILE_EXTRA_EPOCH_VOLCANO))
                    .AddCheckBox("islands", "Islands", pTileExtra->HasFlag(TILE_EXTRA_EPOCH_ISLANDS))
                    .AddTextInput("cycleTime", "Cycle Time (minutes):", ToString(pTileExtra->cycleTime), 5);
                break;
            }

            default:
            {
                ItemInfo* pRainItem = GetItemInfoManager()->GetItemByID(pTileExtra->itemID);
                if (!pRainItem)
                    return;

                db.AddItemPicker("choose", "Item: `2" + pItem->name + "``", "Select any item to rain down")
                    .AddTextInput("graivty", "Gravity:", ToString(pTileExtra->gravity), 5)
                    .AddCheckBox("spin", "Spin Item", pTileExtra->HasFlag(TILE_EXTRA_STUFF_SPIN))
                    .AddCheckBox("invert", "Invert Sky Color", pTileExtra->HasFlag(TILE_EXTRA_STUFF_INVERT));
                break;
            }
        }
    }

    Vector2Int& vTilePos = pTile->GetPos();
    db.AddSpacer()
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y)
        .EndDialog("weatherspcl", "Okay", "Close");

    pPlayer->SendOnDialogRequest(db.Get());
}

void WeatherSpecialDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
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

    TileExtra_WeatherSpecial* pTileExtra = pTile->GetExtra<TileExtra_WeatherSpecial>();
    if (!pTileExtra)
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if (!pItem)
        return;

    if (pItem->type != ITEM_TYPE_WEATHER_SPECIAL && pItem->type != ITEM_TYPE_WEATHER_SPECIAL2)
        return;

    if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        return;

    if (pItem->type == ITEM_TYPE_WEATHER_SPECIAL)
    {
        switch (pItem->id)
        {
            case ITEM_ID_WEATHER_MACHINE_HEATWAVE:
            {
                auto pRed = packet.Find("red"_hash);
                auto pGreen = packet.Find("green"_hash);
                auto pBlue = packet.Find("blue"_hash);

                if (!pRed || !pGreen || !pBlue)
                    return;

                int32 red = 0;
                int32 green = 0;
                int32 blue = 0;

                if (!pRed->GetInt(red) != TO_INT_SUCCESS)
                    return;

                if (!pGreen->GetInt(green) != TO_INT_SUCCESS)
                    return;

                if (!pBlue->GetInt(blue) != TO_INT_SUCCESS)
                    return;

                red = Clamp(red, 0, 255);
                green = Clamp(green, 0, 255);
                blue = Clamp(blue, 0, 255);

                if (red < 40 && green < 40 && blue < 40)
                {
                    pPlayer->SendOnTalkBubble("You can't make a heatwave that dark (one of the colors must be 40+)!",
                                              false);
                    return;
                }

                int32 newColor = TO_COLOR_RGB(red, green, blue);
                if (newColor != pTileExtra->color)
                {
                    pTileExtra->color = TO_COLOR_RGB(red, green, blue);
                    pWorld->SendTileUpdate(pTile);
                    pWorld->SendCurrentWeatherToAll();
                }

                break;
            }

            case ITEM_ID_WEATHER_MACHINE_BACKGROUND:
            {
                auto pChoose = packet.Find("choose"_hash);
                if (!pChoose)
                    return;

                int32 chosenItemID;
                if (pChoose->GetInt(chosenItemID) != TO_INT_SUCCESS)
                    return;

                ItemInfo* pChosenItem = GetItemInfoManager()->GetItemByID(chosenItemID);
                if (!pChosenItem || pChosenItem->HasFlag(ITEM_FLAG_MOD))
                    return;

                if (pPlayer->GetInventory().GetCountOfItem(pChosenItem->id) < 1)
                    return;

                if (!pChosenItem->IsBackground())
                {
                    pPlayer->SendOnTalkBubble("That's not a background!", false);
                    return;
                }

                if (pTileExtra->itemID != pItem->id)
                {
                    if (pItem->id == ITEM_ID_CAVE_BACKGROUND && pTileExtra->itemID == ITEM_ID_BLANK)
                        return;

                    pTileExtra->itemID = pItem->id;
                    pWorld->SendTileUpdate(pTile);
                    pWorld->SendCurrentWeatherToAll();
                }
                break;
            }
        }
    }
    else if (pItem->type == ITEM_TYPE_WEATHER_SPECIAL2)
    {
        switch (pItem->id)
        {
            case ITEM_ID_EPOCH_MACHINE:
            {

                break;
            }

            default:
            {
                bool hasChanged = false;

                if (auto pChoose = packet.Find("choose"_hash))
                {
                    int32 itemID;
                    if (!pChoose->GetInt(itemID) != TO_INT_SUCCESS)
                        return;

                    ItemInfo* pChosenItem = GetItemInfoManager()->GetItemByID(itemID);
                    if (!pChosenItem)
                        return;

                    if (pPlayer->GetInventory().GetCountOfItem(pChosenItem->id) > 0)
                    {
                        pTileExtra->itemID = itemID;
                        hasChanged = true;
                    }
                }

                if (auto pGravity = packet.Find("gravity"_hash))
                {
                    int32 gravity;
                    if (pGravity->GetInt(gravity) != TO_INT_SUCCESS)
                        return;

                    if (gravity != pTileExtra->gravity)
                    {
                        gravity = Clamp(gravity, -500, 500);
                        pTileExtra->gravity = gravity;
                        hasChanged = true;
                    }
                }

                if (auto pSpin = packet.Find("spin"_hash))
                {
                    bool val;
                    if (pSpin->GetBool(val) != TO_INT_SUCCESS)
                        return;

                    if (val != pTileExtra->HasFlag(TILE_EXTRA_STUFF_SPIN))
                    {
                        pTileExtra->SetFlag(TILE_EXTRA_STUFF_SPIN);
                        hasChanged = true;
                    }
                }

                if (auto pInvert = packet.Find("invert"_hash))
                {
                    bool val;
                    if (pInvert->GetBool(val) != TO_INT_SUCCESS)
                        return;

                    if (val != pTileExtra->HasFlag(TILE_EXTRA_STUFF_INVERT))
                    {
                        pTileExtra->SetFlag(TILE_EXTRA_STUFF_INVERT);
                        hasChanged = true;
                    }
                }

                if (hasChanged)
                {
                    pWorld->SendTileUpdate(pTile);
                    pWorld->SendCurrentWeatherToAll();
                }
                break;
            }
        }
    }
}

void XenoniteDialog::Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem)
{
    if (!pPlayer || !pTile || !pItem)
        return;

    if (pItem->type != ITEM_TYPE_XENONITE)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    TileExtra_Xenonite* pTileExtra = pTile->GetExtra<TileExtra_Xenonite>();
    if (!pTileExtra)
        return;

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
        .AddLabelWithIcon(pItem->name, pItem->id, true)
        .EmbedData("tilex", vTilePos.x)
        .EmbedData("tiley", vTilePos.y);

    if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
    {
        if (pTileExtra->HasFlag(TILE_EXTRA_XENO_F_DOUBLE_JUMP))
            db.AddTextBox("`2Double Jump power is given to all players.``");
        else if (pTileExtra->HasFlag(TILE_EXTRA_XENO_B_DOUBLE_JUMP))
            db.AddTextBox("`6Double Jump power is blocked for all players.``");
        else
            db.AddTextBox("Double Jump power can be used if equipped.");

        if (pTileExtra->HasFlag(TILE_EXTRA_XENO_F_HIGH_JUMP))
            db.AddTextBox("`2High Jump power is given to all players.``");
        else if (pTileExtra->HasFlag(TILE_EXTRA_XENO_B_HIGH_JUMP))
            db.AddTextBox("`6High Jump power is blocked for all players.``");
        else
            db.AddTextBox("High Jump power can be used if equipped.");

        if (pTileExtra->HasFlag2(TILE_EXTRA_XENO_F_HEAT_RESIST))
            db.AddTextBox("`2Heat Resist power is given to all players.``");
        else if (pTileExtra->HasFlag2(TILE_EXTRA_XENO_B_HEAT_RESIST))
            db.AddTextBox("`6Heat Resist power is blocked for all players.``");
        else
            db.AddTextBox("Heat Resist power can be used if equipped.");

        if (pTileExtra->HasFlag(TILE_EXTRA_XENO_F_STRONG_PUNCH))
            db.AddTextBox("`2Strong Punch power is given to all players.``");
        else if (pTileExtra->HasFlag(TILE_EXTRA_XENO_B_STRONG_PUNCH))
            db.AddTextBox("`6Strong Punch power is blocked for all players.``");
        else
            db.AddTextBox("Strong Punch power can be used if equipped.");

        if (pTileExtra->HasFlag2(TILE_EXTRA_XENO_F_LONG_PUNCH))
            db.AddTextBox("`2Long Punch power is given to all players.``");
        else if (pTileExtra->HasFlag2(TILE_EXTRA_XENO_B_LONG_PUNCH))
            db.AddTextBox("`6Long Punch power is blocked for all players.``");
        else
            db.AddTextBox("Long Punch power can be used if equipped.");

        if (pTileExtra->HasFlag(TILE_EXTRA_XENO_F_SPEED))
            db.AddTextBox("`2Speedy power is given to all players.``");
        else if (pTileExtra->HasFlag(TILE_EXTRA_XENO_B_SPEED))
            db.AddTextBox("`6Speedy power is blocked for all players.``");
        else
            db.AddTextBox("Speedy power can be used if equipped.");

        if (pTileExtra->HasFlag2(TILE_EXTRA_XENO_F_LONG_BUILD))
            db.AddTextBox("`2Long Build power is given to all players.``");
        else if (pTileExtra->HasFlag2(TILE_EXTRA_XENO_B_LONG_BUILD))
            db.AddTextBox("`6Long Build power is blocked for all players.``");
        else
            db.AddTextBox("Long Build power can be used if equipped.");

        if (pTileExtra->HasFlag2(TILE_EXTRA_XENO_B_POWERUPS))
            db.AddTextBox("`6Temporary powerups like Balloons and Coffee are not usable.``");

        db.EndDialog("xenonite_edit", "", "OK");
    }
    else
    {
        db.AddTextBox(
              "This crystal can either grant or block super powers for all players in your  world! Any power that\'s "
              "unchecked will work as normal - people will have the power if they equip an item with it.")
            .AddSpacer();

        db.AddCheckBox("checkbox_force_dbl", "Force Double Jump", pTileExtra->HasFlag(TILE_EXTRA_XENO_F_DOUBLE_JUMP))
            .AddCheckBox("checkbox_block_dbl", "Block Double Jump", pTileExtra->HasFlag(TILE_EXTRA_XENO_B_DOUBLE_JUMP))
            .AddSpacer();

        db.AddCheckBox("checkbox_force_hig", "Force High Jump", pTileExtra->HasFlag(TILE_EXTRA_XENO_F_HIGH_JUMP))
            .AddCheckBox("checkbox_block_hig", "Block High Jump", pTileExtra->HasFlag(TILE_EXTRA_XENO_B_HIGH_JUMP))
            .AddSpacer();

        db.AddCheckBox("checkbox_force_asb", "Force Heat Resist", pTileExtra->HasFlag2(TILE_EXTRA_XENO_F_HEAT_RESIST))
            .AddCheckBox("checkbox_block_asb", "Block Heat Resist", pTileExtra->HasFlag2(TILE_EXTRA_XENO_B_HEAT_RESIST))
            .AddSpacer();

        db.AddCheckBox("checkbox_force_pun", "Force Strong Punch", pTileExtra->HasFlag(TILE_EXTRA_XENO_F_STRONG_PUNCH))
            .AddCheckBox("checkbox_block_pun", "Block Strong Punch",
                         pTileExtra->HasFlag(TILE_EXTRA_XENO_B_STRONG_PUNCH))
            .AddSpacer();

        db.AddCheckBox("checkbox_force_lng", "Force Long Punch", pTileExtra->HasFlag2(TILE_EXTRA_XENO_F_LONG_PUNCH))
            .AddCheckBox("checkbox_block_lng", "Block Long Punch", pTileExtra->HasFlag2(TILE_EXTRA_XENO_B_LONG_PUNCH))
            .AddSpacer();

        db.AddCheckBox("checkbox_force_spd", "Force Speedy", pTileExtra->HasFlag(TILE_EXTRA_XENO_F_SPEED))
            .AddCheckBox("checkbox_block_spd", "Block Speedy", pTileExtra->HasFlag(TILE_EXTRA_XENO_B_SPEED))
            .AddSpacer();

        db.AddCheckBox("checkbox_force_lngb", "Force Long Build", pTileExtra->HasFlag2(TILE_EXTRA_XENO_F_LONG_BUILD))
            .AddCheckBox("checkbox_block_lngb", "Block Long Build", pTileExtra->HasFlag2(TILE_EXTRA_XENO_B_LONG_BUILD))
            .AddSpacer();

        db.AddCheckBox("checkbox_block_pwr", "Block Use of Powerups", pTileExtra->HasFlag2(TILE_EXTRA_XENO_B_POWERUPS))
            .AddSpacer();

        db.EndDialog("xenonite_edit", "Update", "Cancel");
    }

    pPlayer->SendOnDialogRequest(db.Get());
}

void XenoniteDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
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

    TileExtra_Xenonite* pTileExtra = pTile->GetExtra<TileExtra_Xenonite>();
    if (!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The crystal is gone!", false);
        return;
    }

    if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
    {
        pPlayer->SendOnTalkBubble("No hacking the crystal!", false);
        return;
    }

    pTileExtra->flags = 0;
    pTileExtra->flags2 = 0;

    auto pForceDbl = packet.Find("checkbox_force_dbl"_hash);
    auto pBlockDbl = packet.Find("checkbox_block_dbl"_hash);
    if (pForceDbl || pBlockDbl)
    {
        bool force = false, block = false;
        if (pForceDbl && pForceDbl->GetBool(force) != TO_INT_SUCCESS)
            return;
        if (pBlockDbl && pBlockDbl->GetBool(block) != TO_INT_SUCCESS)
            return;

        force ? pTileExtra->SetFlag(TILE_EXTRA_XENO_F_DOUBLE_JUMP)
              : pTileExtra->RemoveFlag(TILE_EXTRA_XENO_F_DOUBLE_JUMP);
        block ? pTileExtra->SetFlag(TILE_EXTRA_XENO_B_DOUBLE_JUMP)
              : pTileExtra->RemoveFlag(TILE_EXTRA_XENO_B_DOUBLE_JUMP);
    }

    auto pForceHig = packet.Find("checkbox_force_hig"_hash);
    auto pBlockHig = packet.Find("checkbox_block_hig"_hash);
    if (pForceHig || pBlockHig)
    {
        bool force = false, block = false;
        if (pForceHig && pForceHig->GetBool(force) != TO_INT_SUCCESS)
            return;
        if (pBlockHig && pBlockHig->GetBool(block) != TO_INT_SUCCESS)
            return;

        force ? pTileExtra->SetFlag(TILE_EXTRA_XENO_F_HIGH_JUMP) : pTileExtra->RemoveFlag(TILE_EXTRA_XENO_F_HIGH_JUMP);
        block ? pTileExtra->SetFlag(TILE_EXTRA_XENO_B_HIGH_JUMP) : pTileExtra->RemoveFlag(TILE_EXTRA_XENO_B_HIGH_JUMP);
    }

    auto pForceAsb = packet.Find("checkbox_force_asb"_hash);
    auto pBlockAsb = packet.Find("checkbox_block_asb"_hash);
    if (pForceAsb || pBlockAsb)
    {
        bool force = false, block = false;
        if (pForceAsb && pForceAsb->GetBool(force) != TO_INT_SUCCESS)
            return;
        if (pBlockAsb && pBlockAsb->GetBool(block) != TO_INT_SUCCESS)
            return;

        force ? pTileExtra->SetFlag2(TILE_EXTRA_XENO_F_HEAT_RESIST)
              : pTileExtra->RemoveFlag2(TILE_EXTRA_XENO_F_HEAT_RESIST);
        block ? pTileExtra->SetFlag2(TILE_EXTRA_XENO_B_HEAT_RESIST)
              : pTileExtra->RemoveFlag2(TILE_EXTRA_XENO_B_HEAT_RESIST);
    }

    auto pForcePun = packet.Find("checkbox_force_pun"_hash);
    auto pBlockPun = packet.Find("checkbox_block_pun"_hash);
    if (pForcePun || pBlockPun)
    {
        bool force = false, block = false;
        if (pForcePun && pForcePun->GetBool(force) != TO_INT_SUCCESS)
            return;
        if (pBlockPun && pBlockPun->GetBool(block) != TO_INT_SUCCESS)
            return;

        force ? pTileExtra->SetFlag(TILE_EXTRA_XENO_F_STRONG_PUNCH)
              : pTileExtra->RemoveFlag(TILE_EXTRA_XENO_F_STRONG_PUNCH);
        block ? pTileExtra->SetFlag(TILE_EXTRA_XENO_B_STRONG_PUNCH)
              : pTileExtra->RemoveFlag(TILE_EXTRA_XENO_B_STRONG_PUNCH);
    }

    auto pForceLng = packet.Find("checkbox_force_lng"_hash);
    auto pBlockLng = packet.Find("checkbox_block_lng"_hash);
    if (pForceLng || pBlockLng)
    {
        bool force = false, block = false;
        if (pForceLng && pForceLng->GetBool(force) != TO_INT_SUCCESS)
            return;
        if (pBlockLng && pBlockLng->GetBool(block) != TO_INT_SUCCESS)
            return;

        force ? pTileExtra->SetFlag2(TILE_EXTRA_XENO_F_LONG_PUNCH)
              : pTileExtra->RemoveFlag2(TILE_EXTRA_XENO_F_LONG_PUNCH);
        block ? pTileExtra->SetFlag2(TILE_EXTRA_XENO_B_LONG_PUNCH)
              : pTileExtra->RemoveFlag2(TILE_EXTRA_XENO_B_LONG_PUNCH);
    }

    auto pForceSpd = packet.Find("checkbox_force_spd"_hash);
    auto pBlockSpd = packet.Find("checkbox_block_spd"_hash);
    if (pForceSpd || pBlockSpd)
    {
        bool force = false, block = false;
        if (pForceSpd && pForceSpd->GetBool(force) != TO_INT_SUCCESS)
            return;
        if (pBlockSpd && pBlockSpd->GetBool(block) != TO_INT_SUCCESS)
            return;

        force ? pTileExtra->SetFlag(TILE_EXTRA_XENO_F_SPEED) : pTileExtra->RemoveFlag(TILE_EXTRA_XENO_F_SPEED);
        block ? pTileExtra->SetFlag(TILE_EXTRA_XENO_B_SPEED) : pTileExtra->RemoveFlag(TILE_EXTRA_XENO_B_SPEED);
    }

    auto pForceLngb = packet.Find("checkbox_force_lngb"_hash);
    auto pBlockLngb = packet.Find("checkbox_block_lngb"_hash);
    if (pForceLngb || pBlockLngb)
    {
        bool force = false, block = false;
        if (pForceLngb && pForceLngb->GetBool(force) != TO_INT_SUCCESS)
            return;
        if (pBlockLngb && pBlockLngb->GetBool(block) != TO_INT_SUCCESS)
            return;

        force ? pTileExtra->SetFlag2(TILE_EXTRA_XENO_F_LONG_BUILD)
              : pTileExtra->RemoveFlag2(TILE_EXTRA_XENO_F_LONG_BUILD);
        block ? pTileExtra->SetFlag2(TILE_EXTRA_XENO_B_LONG_BUILD)
              : pTileExtra->RemoveFlag2(TILE_EXTRA_XENO_B_LONG_BUILD);
    }

    if (auto pBlockPwr = packet.Find("checkbox_block_pwr"_hash))
    {
        bool block = false;
        if (pBlockPwr->GetBool(block) != TO_INT_SUCCESS)
            return;

        block ? pTileExtra->SetFlag2(TILE_EXTRA_XENO_B_POWERUPS) : pTileExtra->RemoveFlag2(TILE_EXTRA_XENO_B_POWERUPS);
    }

    pWorld->SendConsoleMessageToAll("The Xenonite Crystal has shifted...");
    pWorld->ToggleXenoniteCrystal(true);
}