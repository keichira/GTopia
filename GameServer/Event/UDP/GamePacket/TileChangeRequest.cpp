#include "TileChangeRequest.h"
#include "../../../Item/HarmonicCrystal.h"
#include "../../../Player/Dialog/DisplayBlockDialog.h"
#include "../../../Player/Dialog/DonationBoxDialog.h"
#include "../../../Player/Dialog/DressupDialog.h"
#include "../../../Player/Dialog/MannequinDialog.h"
#include "../../../Player/Dialog/PlayerDialog.h"
#include "Item/ItemInfoManager.h"
#include "Utils/GrowUtils.h"

void TileChangeRequest::Execute(GamePlayer* pPlayer, World* pWorld, GameUpdatePacket* pPacket)
{
    if (!pPlayer || !pWorld || !pPacket)
        return;

    Role* pRole = pPlayer->GetRole();
    if (!pRole)
        return;

    Timer& lastTileChangeTime = pPlayer->GetLastTileChangeTime();
    if (!lastTileChangeTime.IsPassed())
        return;

    lastTileChangeTime.Set(pPacket->field_7 == ITEM_ID_FIST ? 130 : 70);

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pPacket->field_7);
    if (!pItem)
    {
        pPlayer->SendFakePingReply();
        return;
    }

    if (pItem->HasFlag(ITEM_FLAG_MOD) && !pRole->HasPerm("bypass.item_mod"_hash))
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(pPacket->field_11, pPacket->field_12);
    if (!pTile)
        return;

    TileInfo* pPlayerTile = pWorld->GetTileManager()->GetTileByWorldPos(pPlayer->GetWorldPos());
    if (!pPlayerTile)
        return;

    bool isFromSuckerRemote = (pItem->type == ITEM_TYPE_PLANTER);

    PlayerInventory& inventory = pPlayer->GetInventory();

    if (isFromSuckerRemote)
    {
        TileInfo* pSuckerTile = pWorld->GetSuckerBlockManager().GetActivePlanter();
        if (!pSuckerTile)
        {
            pPlayer->SendOnTalkBubble("There's no active planter!", false);
            return;
        }

        TileExtra_Sucker* pSuckerExtra = pSuckerTile->GetExtra<TileExtra_Sucker>();
        if (!pSuckerExtra || pSuckerExtra->itemID == ITEM_ID_BLANK)
            return;

        if (pSuckerExtra->itemID != ITEM_ID_BLANK)
        {
            if (!SuckerBlockManager::IsAllowedToBuildOrPlantItem(pSuckerExtra->itemID))
            {
                if (ItemInfo* pSuckerItemInfo = GetItemInfoManager()->GetItemByID(pSuckerTile->GetFG()))
                {
                    pPlayer->SendOnTalkBubble("There's no active " + pSuckerItemInfo->name + "!", false);
                }
                return;
            }

            if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
            {
                if (ItemInfo* pSuckedItemInfo = GetItemInfoManager()->GetItemByID(pSuckerExtra->itemID))
                {
                    if (pSuckedItemInfo->type == ITEM_TYPE_SEED)
                    {
                        pPlayer->SendOnTalkBubble("You don't have the access to plant there", false);
                    }
                    else
                    {
                        pPlayer->SendOnTalkBubble("You don't have the access to build there!", false);
                    }
                }
                return;
            }

            if (pSuckerExtra->count < 1)
            {
                if (ItemInfo* pSuckerItemInfo = GetItemInfoManager()->GetItemByID(pSuckerTile->GetFG()))
                {
                    pPlayer->SendOnTalkBubble("The " + pSuckerItemInfo->name + " is empty!", false);
                }
            }

            pPacket->field_7 = pSuckerExtra->itemID;
        }
    }

    pItem = GetItemInfoManager()->GetItemByID(pPacket->field_7);
    if (!pItem)
        return;

    if (pItem->id == ITEM_ID_FIST)
    {
        uint16 handItemID = inventory.GetClothByPart(BODY_PART_HAND);
        uint16 backItemID = inventory.GetClothByPart(BODY_PART_BACK);

        if (handItemID == ITEM_ID_PARTY_BUBBLE_BLASTER)
        {
            pPlayer->SendFakePingReply();
            return;
        }

        if ((handItemID == ITEM_ID_CHAINSAW_HAND && pTile->GetFG() == ITEM_ID_ICE_SCULPTURES) ||
            (handItemID == ITEM_ID_GARDEN_SHEARS &&
             (pTile->GetFG() == ITEM_ID_TOPIARY_HEDGE || pTile->GetFG() == ITEM_ID_SPOOKY_BUNTING)))
        {
            if (!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
                return;

            if (pTile->HasFlag(TILE_FLAG_BG_IS_ON))
            {
                pTile->RemoveFlag(TILE_FLAG_BG_IS_ON);

                if (pTile->HasFlag(TILE_FLAG_FG_ALT_MODE))
                    pTile->RemoveFlag(TILE_FLAG_FG_ALT_MODE);
                else
                    pTile->SetFlag(TILE_FLAG_FG_ALT_MODE);
            }
            else
            {
                pTile->SetFlag(TILE_FLAG_BG_IS_ON);

                if (pTile->HasFlag(TILE_FLAG_FG_ALT_MODE))
                    pTile->SetFlag(TILE_FLAG_FG_ALT_MODE);
                else
                    pTile->RemoveFlag(TILE_FLAG_FG_ALT_MODE);
            }

            pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_SHRAPNEL_BOOM, pTile->GetWorldPosCenter());
            pWorld->SendTileUpdate(pTile);
            return;
        }

        if (IsFuelPack(backItemID) && handItemID != ITEM_ID_FLAMETHROWER)
        {
            pPlayer->SendFakePingReply();
            return;
        }

        if (handItemID == ITEM_ID_CLOAK_OF_FALLING_WATERS || handItemID == ITEM_ID_SUPER_SQUIRT_RIFLE_500 ||
            handItemID == ITEM_ID_FIRE_HOSE)
        {
            if (pTile->HasFlag(TILE_FLAG_ON_FIRE))
            {
                pWorld->PutOutFire(pTile, pPlayer);
                pWorld->SendTileUpdate(pTile);
            }
            else
            {
                pPlayer->SendFakePingReply();
            }

            return;
        }

        if (handItemID == ITEM_ID_NEUTRON_GUN && backItemID == ITEM_ID_NEUTRON_PACK)
        {
            pWorld->GetNPCManager()->OnNeutronBeam(pPlayer, pPlayer->GetWorldPosCenter(), pTile->GetWorldPosCenter());
            pPlayer->SendFakePingReply();
            return;
        }

        if (handItemID == ITEM_ID_SUSHI_KNIFE || handItemID == ITEM_ID_BUTCHER_KNIFE)
        {
            if (pTile->GetFG() == ITEM_ID_CUTTING_BOARD)
            {
                auto itemsInRect = pWorld->GetObjectManager()->GetObjectsInRect(pTile->GetRect());
                if (!itemsInRect.empty())
                {
                    for (uint32 i = 0; i < itemsInRect.size(); ++i)
                    {
                        WorldObject* pObj = itemsInRect[i];
                        if (!pObj)
                            continue;

                        ItemInfo* pObjItem = GetItemInfoManager()->GetItemByID(pObj->itemID);
                        if (!pObjItem)
                            continue;

                        // todo fish check
                    }
                }
            }
        }

        if (pTile->GetBG() == ITEM_ID_CHOPPING_WOOD_BLOCK &&
            (handItemID == ITEM_ID_HEADSMANS_AXE || handItemID == ITEM_ID_LUMBER_AXE || handItemID == ITEM_ID_FIRE_AX))
        {
            // todo chopping wood
            return;
        }

        if (pTile->GetFG() == ITEM_ID_GIVING_TREE && handItemID == ITEM_ID_LUMBER_AXE)
        {

            return;
        }

        if (IsGaunletOfElements(handItemID))
        {
        }
    }

    if (pTile->GetFG() == ITEM_ID_RUNE_CARVED_DOOR && !pRole->HasPerm("bypass.item_mod"_hash))
    {
        pPlayer->SendFakePingReply();
        return;
    }

    if (pTile->HasFlag(TILE_FLAG_ON_FIRE) &&
        (pItem->id != ITEM_ID_WATER_BALLOON && pItem->id != ITEM_ID_WATER_BUCKET &&
         pItem->id != ITEM_ID_SMALL_WAR_BALLOON && pItem->id != ITEM_ID_MEDIUM_WAR_BALLOON &&
         pItem->id != ITEM_ID_LARGE_WAR_BALLOON))
    {
        pPlayer->SendFakePingReply();
        return;
    }

    if (pItem->id != ITEM_ID_FIST && pItem->id == ITEM_ID_ANGELIC_COUNTING_CLOUD &&
        pWorld->GetWorldOwnerID() != pPlayer->GetUserID())
    {
        pPlayer->SendOnTalkBubble("Only the world owner can place these!", false);
        pPlayer->SendFakePingReply();
        return;
    }

    ItemInfo* pTileItem = GetItemInfoManager()->GetItemByID(pTile->GetDisplayedItem());
    if (!pTileItem)
        return;

    /**
     * donation box, mannequin click with item check
     */

    if (pTileItem->type == ITEM_TYPE_DONATION_BOX && pItem->type != ITEM_TYPE_FIST && pItem->type != ITEM_TYPE_WRENCH)
    {
        DonationBoxDialog::RequestDonatingItem(pPlayer, pTile, pItem->id);
        return;
    }

    if (pTileItem->type == ITEM_TYPE_MANNEQUIN && pItem->type != ITEM_TYPE_FIST && pItem->type != ITEM_TYPE_WRENCH)
    {
        auto playersInRect = pWorld->GetPlayersInWorldRect(pTile->GetRect());
        if (!playersInRect.empty())
        {
            pPlayer->SendOnTextOverlay("Somebody is in the way of the " + pTileItem->name + "``!");
        }
        else
        {
            MannequinDialog::RequestPutItem(pPlayer, pTile, pItem->id, false);
        }
        return;
    }

    if (pTileItem->type == ITEM_TYPE_DISPLAY_BLOCK && pItem->type != ITEM_TYPE_FIST && pItem->type != ITEM_TYPE_WRENCH)
    {
        DisplayBlockDialog::RequestPutItem(pPlayer, pTile, pItem->id);
        return;
    }

    if (pTileItem)

        if (pItem->IsBackground() && pItem->id == pTile->GetBG())
            return;

    /**
     * arm check (hack)
     */

    GamePlayer* pTargetPlayer = nullptr;
    if (pItem->id != ITEM_ID_FIST)
    {
        auto playersInRect = pWorld->GetPlayersInWorldRect(pTile->GetRect());
        if (!playersInRect.empty())
        {
            for (auto& pPlayerTarget : playersInRect)
            {
                if (!pPlayerTarget)
                    continue;

                if (pPlayerTarget == pPlayer)
                {
                    pTargetPlayer = pPlayerTarget;
                    break;
                }
            }

            pTargetPlayer = playersInRect[0];
        }
    }

    if (pTileItem->type == ITEM_TYPE_LOCK && pItem->IsBackground())
    {
        TileExtra_Lock* pTileExtraLock = pTile->GetExtra<TileExtra_Lock>();
        if (!pTileExtraLock)
            return;

        if (pTileExtraLock->ownerID != pPlayer->GetUserID())
        {
            pWorld->OnTriedPunchedOrPlaceLockedArea(pPlayer, pTile, true);
            return;
        }
    }

    if ((pItem->id == ITEM_ID_WRENCH || pItem->IsLOSBlocking()) && !pPlayer->HasLOSToTile(pTile))
    {
        pPlayer->SendOnTalkBubble("Something is blocking the way, get closer.", false);
        pWorld->SendPlayPositionedToAll(pPlayer, "punch_locked.wav");
        return;
    }

    /**
     * handle tile punch message
     */
    float tileHealthPercent = pTile->GetHealthPercent();
    if (pItem->id == ITEM_ID_FIST)
    {
        if (pTileItem->type == ITEM_TYPE_ACHIEVEMENT && tileHealthPercent >= 1.0f)
        {
            if (TileExtra_Achievement* pAchiExtra = pTile->GetExtra<TileExtra_Achievement>())
            {
                if (pAchiExtra->achievementID == 127)
                {
                    pPlayer->SendOnTalkBubble("It's blank. Will no hero step up to etch it?", true);
                }
                else
                {
                    if (pAchiExtra->achievementID > ACHIEVEMENT_COUNT ||
                        !GetAchievementManager()->GetAchievement((eAchievement)pAchiExtra->achievementID))
                        pPlayer->SendOnTalkBubble("Invalid achievement.", true);
                    else
                        pWorld->OnPunchedAchievementBlock(pPlayer, pTile, pTileItem);
                }
            }
        }
    }

    bool allowPunchInteraction = false;

    if (pTileItem->type == ITEM_TYPE_BOOMBOX || pTileItem->type == ITEM_TYPE_SFX_WITH_EXTRA_FRAME ||
        pTileItem->type == ITEM_TYPE_BACK_BOOMBOX || pTileItem->type == ITEM_TYPE_COMPONENT ||
        pTileItem->type == ITEM_TYPE_PROVIDER || pTileItem->type == ITEM_TYPE_SCOREBOARD ||
        pTileItem->id == ITEM_ID_SLOT_MACHINE || pTileItem->id == ITEM_ID_ROULETTE_WHEEL ||
        pTileItem->id == ITEM_ID_THE_RINGMASTER || pTileItem->id == ITEM_ID_LOCKE_THE_SALESMAN ||
        pTileItem->id == ITEM_ID_NUTCRACKER || pTileItem->id == ITEM_ID_WALL_CLOCK ||
        pTileItem->id == ITEM_ID_CRYSTAL_CLOCK || pTileItem->id == ITEM_ID_LUXURIOUS_WALL_CLOCK ||
        pTileItem->id == ITEM_ID_STEAM_CRANK || pTileItem->id == ITEM_ID_STUFF_4_TOYS_BOX ||
        pTileItem->id == ITEM_ID_MASTER_PENG)
    {
        allowPunchInteraction = true;

        if ((pTileItem->type == ITEM_TYPE_SWITCHEROO || pTileItem->type == ITEM_TYPE_COMPONENT ||
             pTileItem->id == ITEM_ID_STEAM_CRANK) &&
            !pTile->HasFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC))
        {
            allowPunchInteraction = false;
        }
    }
    else if (pTileItem->type == ITEM_TYPE_DOOR)
    {
        TileInfo* pPlayerTile = pWorld->GetTileManager()->GetTileByWorldPos(pPlayer->GetWorldPos());
        if (pPlayerTile == pTile)
        {
            allowPunchInteraction = true;
        }
    }

    if (pTileItem->type != ITEM_TYPE_LOCK && pWorld->GetTileManager()->IsTileLockedWithLock(pTile))
    {
        if (pItem->type != ITEM_TYPE_CONSUMABLE)
        {
            bool hasAccessToLock = false;
            TileInfo* pParentTile = pWorld->GetTileManager()->GetTileParentTileWithWorldLock(pTile);
            TileExtra_Lock* pLockExtra = pParentTile ? pParentTile->GetExtra<TileExtra_Lock>() : nullptr;

            if (pLockExtra && pLockExtra->HasAccess(pPlayer->GetUserID()))
            {
                hasAccessToLock = true;
            }

            if (!hasAccessToLock && pParentTile)
            {
                if (pParentTile->HasFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC))
                {
                    hasAccessToLock = true;
                }

                if (pParentTile->GetFG() == ITEM_ID_BUILDERS_LOCK)
                {
                    if (!pLockExtra)
                    {
                        hasAccessToLock = false;
                    }
                    else
                    {
                        if (!pLockExtra->HasAccess(pPlayer->GetUserID()))
                        {
                            if (pLockExtra->HasFlag(TILE_EXTRA_LOCK_BUILD_ONLY) && pItem->id == ITEM_ID_FIST)
                            {
                                pPlayer->SendOnTalkBubble("This lock allows building only!", false);
                                return;
                            }

                            if (!pLockExtra->HasFlag(TILE_EXTRA_LOCK_BUILD_ONLY) && pItem->id != ITEM_ID_FIST &&
                                pItem->id != ITEM_ID_WRENCH)
                            {
                                pPlayer->SendOnTalkBubble("This lock allows breaking only!", false);
                                return;
                            }
                        }

                        // todo this no idea what limiting admins do
                        else if (pLockExtra->HasFlag(TILE_EXTRA_LOCK_LIMIT_ADMINS) &&
                                 pLockExtra->ownerID != pPlayer->GetUserID())
                        {
                            hasAccessToLock = false;
                        }
                    }
                }
            }

            if (!hasAccessToLock)
            {
                if (pItem->id == ITEM_ID_FIST)
                {
                    if (!allowPunchInteraction)
                        return;
                }
                else if (pItem->id == ITEM_ID_WRENCH)
                {
                    if (pTileItem->type == ITEM_TYPE_MAILBOX || pTileItem->type == ITEM_TYPE_DONATION_BOX ||
                        pTileItem->type == ITEM_TYPE_MANNEQUIN || pTileItem->type == ITEM_TYPE_DRESSUP)
                    {
                        PlayerDialog::Handle(pPlayer, pTile);
                        return;
                    }

                    pWorld->OnTriedPunchedOrPlaceLockedArea(pPlayer, pParentTile, true);
                    return;
                }
                else
                {
                    pWorld->OnTriedPunchedOrPlaceLockedArea(pPlayer, pParentTile, true);
                    return;
                }
            }

            if (pItem->id != ITEM_ID_FIST && pItem->id != ITEM_ID_WRENCH && pItem->type == ITEM_TYPE_LOCK)
            {
                if (pLockExtra && pLockExtra->ownerID != pPlayer->GetUserID())
                {
                    pPlayer->SendOnTalkBubble("Only the world owner can place more locks here!", false);
                    pWorld->OnTriedPunchedOrPlaceLockedArea(pPlayer, pParentTile, true);
                    return;
                }
            }
        }
    }

    if (pItem->id != ITEM_ID_FIST && pItem->type == ITEM_TYPE_VIP_DOOR)
    {
        if (TileExtra_Lock* pLockExtra = pWorld->GetTileManager()->GetTileParentLockExtra(pTile);
            pLockExtra->ownerID != pPlayer->GetUserID())
        {
            pPlayer->SendOnTalkBubble(
                "Only the world owner can place a VIP Entrance, since only the world owner can destroy one.", false);
            return;
        }
    }

    if (pItem->id == ITEM_ID_WRENCH)
    {
        if (pTile->GetDisplayedItem() != ITEM_ID_BLANK)
        {
            PlayerDialog::Handle(pPlayer, pTile);
        }
        return;
    }

    if (pItem->type == ITEM_TYPE_CONSUMABLE)
    {
        pWorld->OnConsumeConsumable(pPlayer, pTargetPlayer, pTile, pItem);
        return;
    }

    if (pItem->type == ITEM_TYPE_CLOTHES)
    {
        if (pTargetPlayer == pPlayer)
        {
            pPlayer->ToggleCloth(pItem->id);
        }
        else
        {
            // add some more things
            pPlayer->SendOnTalkBubble("To wear clothing, use on yourself", false);
        }

        return;
    }

    if (pItem->type == ITEM_TYPE_ARTIFACT)
    {
        if (pTargetPlayer == pPlayer)
        {
            // todo artifact
        }
        else
        {
            pPlayer->SendOnTalkBubble("To wear artifact, use on yourself", false);
        }

        return;
    }

    bool canPlaceTheItem = (pTargetPlayer == nullptr) ||
                           (pItem->IsBackground() || pItem->type == ITEM_TYPE_SEED || pItem->type == ITEM_TYPE_CRYSTAL);

    // todo here
    if ((!pRole->HasPerm("state.smod"_hash) &&
         (pTileItem->type == ITEM_TYPE_DOOR || pTileItem->type == ITEM_TYPE_BEDROCK)) ||
        !canPlaceTheItem)
    {
        pPlayer->PlaySFX("cant_place_tile.wav");

        /*string errorMsg;
        if(pItem->type != ITEM_TYPE_SEED && pItem->type != ITEM_TYPE_CRYSTAL)
        {
            if(pItem->type == ITEM_TYPE_DOOR)
            {
                errorMsg = "(stand over and punch to use)";
            }
            else
            {
                errorMsg = "It's too strong to break.";
            }

            if(pItem->IsBackground())
            {
                errorMsg = "Can't put anything behind that!";
            }

            if(errorMsg.empty())
            {
                errorMsg = "I can't break this.";
            }
        }
        else if(pItem->type == ITEM_TYPE_SEED)
        {

        }

        if(!errorMsg.empty())
        {
            pPlayer->SendOnTalkBubble(errorMsg, false);
        }*/

        return;
    }

    if (pItem->id != ITEM_ID_FIST)
    {
        if (pTile->GetFG() != ITEM_ID_BLANK && !pItem->IsBackground() && pItem->type != ITEM_TYPE_SEED &&
            pItem->type != ITEM_TYPE_CRYSTAL)
        {
            pPlayer->PlaySFX("cant_place_tile.wav");
            return;
        }

        TileInfo* pParentTile = pWorld->GetTileManager()->GetTileParentTileWithWorldLock(pTile);
        if (pParentTile && pParentTile->GetFG() == ITEM_ID_BUILDERS_LOCK)
        {
            if (TileExtra_Lock* pTileExtraLock = pParentTile->GetExtra<TileExtra_Lock>())
            {
                if (!pTileExtraLock->HasFlag(TILE_EXTRA_LOCK_BUILD_ONLY) &&
                    !pTileExtraLock->HasAccess(pPlayer->GetUserID()))
                {
                    pPlayer->SendOnTalkBubble("This lock allows breaking only!", false);
                    return;
                }
            }
        }

        int32 itemAmount = inventory.GetCountOfItem(pItem->id);
        if (isFromSuckerRemote)
        {
            itemAmount = pWorld->GetSuckerBlockManager().GetItemCountOfPlanter();
        }

        if (itemAmount < 1)
            return;

        if (pItem->HasFlag(ITEM_FLAG_WORLDLOCKED))
        {
            if (!pWorld->GetTileManager()->GetKeyTile(KEY_TILE_WORLD_LOCK))
            {
                pPlayer->SendOnTalkBubble("This item can only be used in World-Locked worlds!", false);
                return;
            }

            if (pWorld->GetTileManager()->IsTileLockedWithLockButPublic(pTile))
            {
                pPlayer->SendOnTalkBubble("This item can't be placed in areas designated as public!", false);
                return;
            }
        }

        if (pItem->HasFlag(ITEM_FLAG_UNTRADEABLE) && pWorld->GetWorldOwnerID() != pPlayer->GetUserID())
        {
            pPlayer->SendOnTextOverlay("Only the world's owner can place Untradeable items.");
            return;
        }

        if (IsJammer(pItem->id) || pItem->type == ITEM_TYPE_WEATHER_MACHINE ||
            pItem->type == ITEM_TYPE_WEATHER_SPECIAL2 || pItem->type == ITEM_TYPE_INFINITY_WEATHER_MACHINE)
        {
            if (pParentTile)
            {
                if (TileExtra_Lock* pTileExtraLock = pParentTile->GetExtra<TileExtra_Lock>();
                    !pTileExtraLock->HasAccess(pPlayer->GetUserID()) &&
                    pParentTile->HasFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC))
                {
                    pPlayer->SendOnTalkBubble(
                        "This item can't be used by strangers in locked areas that are marked public.", false);
                    return;
                }
            }
        }

        if ((pItem->id == ITEM_ID_GUARDIAN_PINEAPPLE &&
             pWorld->GetTileManager()->GetKeyTile(KEY_TILE_GUARD_PINEAPPLE)) ||
            (pItem->id == ITEM_ID_PUNCH_JAMMER && pWorld->GetTileManager()->GetKeyTile(KEY_TILE_PUNCH_JAMMER)) ||
            (pItem->id == ITEM_ID_ZOMBIE_JAMMER && pWorld->GetTileManager()->GetKeyTile(KEY_TILE_ZOMBIE_JAMMER)) ||
            (pItem->id == ITEM_ID_SIGNAL_JAMMER && pWorld->GetTileManager()->GetKeyTile(KEY_TILE_SIGNAL_JAMMER)) ||
            (pItem->id == ITEM_ID_ANTIGRAVITY_GENERATOR &&
             pWorld->GetTileManager()->GetKeyTile(KEY_TILE_ANTIGRAVITY)) ||
            (pItem->id == ITEM_ID_XENONITE_CRYSTAL && pWorld->GetTileManager()->GetKeyTile(KEY_TILE_XENONITE)) ||
            (pItem->id == ITEM_ID_FIRE_HOSE && pWorld->GetTileManager()->GetKeyTile(KEY_TILE_FIREHOUSE)))
        {
            pPlayer->SendFakePingReply();
            pPlayer->SendOnTalkBubble("This world already has a " + pItem->name +
                                          " somewhere on it, installing two would be dangerous!",
                                      true);
            return;
        }

        if (pItem->type == ITEM_TYPE_LOCK)
        {
            pWorld->OnAddLock(pPlayer, pTile, pItem->id);
            return;
        }

        if (pItem->type == ITEM_TYPE_SEED || pTileItem->type == ITEM_TYPE_CRYSTAL)
        {
            pWorld->OnPlantSeed(pPlayer, pTile, pItem, pPacket);
            return;
        }

        if (pItem->id == ITEM_ID_HEART_MONITOR && pWorld->GetTileManager()->GetHeartMonitors().size() >= 30)
        {
            pPlayer->SendOnTalkBubble("``Due to wiring concerns, you can only place 30 Heart Monitors in a world.",
                                      false);
            pWorld->SendPlayPositionedToAll(pPlayer, "punch_locked.wav");
            return;
        }

        /*if(pItem->IsBackground() && pTile->GetBG() != ITEM_ID_BLANK)
        {
            ItemInfo* pTileBgItem = GetItemInfoManager()->GetItemByID(pTile->GetBG());
            if(pTileBgItem && pTileBgItem->HasFlag(ITEM_FLAG_EDITABLE))
            {

            }
        }*/

        pWorld->HandleTilePackets(pPacket);

        pPlayer->GetProgressData().AddProgress(PLAYER_PROGRESS_PLACE_COUNT, 1);
        if (!pItem->IsUnlimited())
        {
            pWorld->RemoveSingleItemFromPlayerOrSucker(pPlayer, pItem->id, isFromSuckerRemote);
        }

        if (pItem->type == ITEM_TYPE_ACHIEVEMENT)
        {
            TileExtra_Achievement* pAchiExtra = pTile->GetExtra<TileExtra_Achievement>();
            if (pAchiExtra)
            {
                pAchiExtra->ownerID = pPlayer->GetUserID();
            }
        }

        if (pItem->type == ITEM_TYPE_CRYSTAL)
        {
            TileExtra_Crystal* pCrystalExtra = pTile->GetExtra<TileExtra_Crystal>();
            if (pCrystalExtra)
            {
                pCrystalExtra->crystals += gHarmonicCrystal.GetCrystalCodeFromID(pItem->id);
            }
        }

        if (pItem->type == ITEM_TYPE_XENONITE)
        {
            pWorld->ToggleXenoniteCrystal(true);
        }

        if (pItem->id == ITEM_ID_HEART_MONITOR)
        {
            if (TileExtra_HeartMonitor* pMonitorExtra = pTile->GetExtra<TileExtra_HeartMonitor>())
            {
                pMonitorExtra->ownerID = pPlayer->GetUserID();
                pMonitorExtra->playerDisplayName = pPlayer->GetDisplayName(false) + "`w";
                pWorld->OnHeartMonitorAdded(pTile);
            }
            pTile->SetFlag(TILE_FLAG_IS_ON);
        }

        if (pTile->HasFlag(TILE_FLAG_PAINTED_WHITE))
        {
            pTile->RemoveFlag(TILE_FLAG_PAINTED_WHITE);
        }
    }
    else
    {
        uint32 displayedItemID = pTile->GetDisplayedItem();

        if (!pRole->HasPerm("state.smod"_hash) &&
            (displayedItemID == ITEM_ID_MAGIC_INFUSED_VEIN || displayedItemID == ITEM_ID_PURE_MAGIC_ORE))
        {
            pPlayer->SendOnTalkBubble(
                "Hmmm, turns out something can stand up to the power of the FIST! Better try something stronger.",
                false);
            return;
        }

        TileInfo* pParentTile = pWorld->GetTileManager()->GetTileParentTileWithWorldLock(pTile);
        if (pParentTile && pParentTile->GetFG() == ITEM_ID_BUILDERS_LOCK)
        {
            if (TileExtra_Lock* pTileExtraLock = pParentTile->GetExtra<TileExtra_Lock>())
            {
                if (pTileExtraLock->HasFlag(TILE_EXTRA_LOCK_BUILD_ONLY) &&
                    !pTileExtraLock->HasAccess(pPlayer->GetUserID()))
                {
                    pPlayer->SendOnTalkBubble("This lock allows building only!", false);
                    return;
                }
            }
        }

        if ((pItem->type == ITEM_TYPE_DOOR || pItem->type == ITEM_TYPE_USER_DOOR) &&
            pTile != pWorld->GetTileManager()->GetTileByWorldPos(pPlayer->GetWorldPos()))
        {
            pPlayer->SendFakePingReply();
            return;
        }

        // lastTileChangeTime.Set(130);

        /**
         * handle punching special tiles like mannequin, lobster traps
         *
         */

        if (pTileItem->type == ITEM_TYPE_MANNEQUIN && MannequinDialog::RequestRemoveItem(pPlayer, pTile))
            return;

        if (pTileItem->type == ITEM_TYPE_DRESSUP && pPlayer->GetTilePlayerOn() == pTile &&
            DressupDialog::RequestPunch(pPlayer, pTile))
            return;

        tileHealthPercent = pTile->GetHealthPercent();

        if (pTileItem->type == ITEM_TYPE_SEED)
        {
            if (pTile->GetGrowthPercent() >= 100.0f)
            { // fix ungrown harvest
                if (pTileItem->rarity < 999)
                {
                    pPlayer->GiveXP(pTileItem->rarity / 5 + 1);
                }

                pPlayer->GetProgressData().AddProgress(PLAYER_PROGRESS_HARVEST_COUNT, 1);
                pWorld->OnHarvestTree(pPlayer, pTile);
                return;
            }
        }

        if (pTileItem->type == ITEM_TYPE_PROVIDER)
        {
            if (pTile->GetGrowthPercent() >= 100.0f)
            {
                pWorld->OnCollectProvider(pPlayer, pTile);
            }
            return;
        }

        if (pTileItem->type == ITEM_TYPE_LOCK)
        {
            TileExtra_Lock* pLockExtra = pTile->GetExtra<TileExtra_Lock>();
            if (!pLockExtra)
            {
                pPlayer->SendFakePingReply();
                return;
            }

            if (pLockExtra->ownerID != pPlayer->GetUserID())
            {
                pWorld->OnTriedPunchedOrPlaceLockedArea(pPlayer, pTile, false);
                return;
            }

            if (IsWorldLock(pTile->GetFG()) &&
                pWorld->GetTileManager()->GetTileInfoFlaggedWith(ITEM_FLAG_UNTRADEABLE, ITEM_ID_MY_FIRST_WORLD_LOCK))
            {
                pPlayer->SendOnTalkBubble("Can't smash the World Lock while Untradeable blocks exist!", false);
                return;
            }

            if (pTile->GetFG() == ITEM_ID_GUILD_LOCK)
            {
                pPlayer->SendOnTalkBubble(
                    "You can't smash a Guild Lock, the only way to destroy it is to abandon the guild!", false);
                return;
            }
        }

        if (pTileItem->HasFlag(ITEM_FLAG_AUTOPICKUP) && !inventory.HaveRoomForItem(pTileItem->id, 1) &&
            tileHealthPercent < 1.0f)
        {
            pPlayer->SendOnTalkBubble("I better not break that, I have no room to pick it up!", false);
            return;
        }

        if (pTile->HasFlag(ITEM_FLAG_UNTRADEABLE) && tileHealthPercent > 0.0f &&
            !pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
        {
            pPlayer->SendOnTalkBubble("Only the world owner can break Untradeable blocks!", false);
            return;
        }

        if (pTileItem->type == ITEM_TYPE_DONATION_BOX)
        {
            if (TileExtra_DonaitonBox* pDonationExtra = pTile->GetExtra<TileExtra_DonaitonBox>())
            {
                if (!pDonationExtra->gifts.empty())
                {
                    pPlayer->SendOnTalkBubble("Empty the " + pTileItem->name + " before smashing!", false);
                    return;
                }
            }
        }

        if (pTileItem->HasFlag(ITEM_FLAG_AUTOPICKUP) && !inventory.HaveRoomForItem(pTileItem->id, 1) &&
            tileHealthPercent < 1.0f)
        {
            pPlayer->SendOnTalkBubble("I better not break that, I have no room to pick it up!", false);
            return;
        }

        if (pTileItem->type == ITEM_TYPE_DISPLAY_BLOCK)
        {
            if (TileExtra_DisplayBlock* pTileExtra = pTile->GetExtra<TileExtra_DisplayBlock>())
            {
                if (pTileExtra->itemID != ITEM_ID_BLANK &&
                    pWorld->GetTileManager()->IsPlayerOwnerOfTheTile(pTile, pPlayer->GetUserID()))
                {
                    pPlayer->SendOnTalkBubble("Only the block's owner can break it!", false);
                    return;
                }
            }
        }

        /**
         * handle things that dont allow to break like empty the box
         */

        // bee

        uint32 punchDamage = pPlayer->GetCharData().punchDamage;
        pTile->PunchTile(punchDamage);

        if (pTileItem->type == ITEM_TYPE_LAB)
        {
        }
        else if (pTileItem->id == ITEM_ID_CASH_REGISTER)
        {
            pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_COIN,
                                            pTile->GetWorldPosCenter() + RandomRangeFloat(250.0, 250.0), 250, 30);
        }
        else if (pTileItem->id == ITEM_ID_ROULETTE_WHEEL)
        {
            int32 randVal = RandomRangeInt(0, 36);
            string spinResult = "`7[``" + pPlayer->GetDisplayName(true) + "`` spun the wheel and got ";

            if (randVal == 0)
            {
                spinResult += "`2";
            }
            else
            {
                bool even = (randVal % 2) == 0;
                bool firstGroup = (randVal >= 1 && randVal <= 10) || (randVal >= 19 && randVal <= 28);

                if (firstGroup)
                    spinResult += even ? "`b" : "`4";
                else
                    spinResult += even ? "`4" : "`b";
            }

            spinResult += ToString(randVal) + "`7]``";

            pWorld->SendTalkBubbleAndConsoleToAll(spinResult, false, pPlayer);
        }
        else if (pTileItem->id == ITEM_ID_SLOT_MACHINE)
        {
            int32 randVal = RandomRangeInt(0, 9);
            if (randVal == 0)
            {
                pWorld->SendTalkBubbleAndConsoleToAll(
                    "`7[``" + pPlayer->GetDisplayName(true) + " `2wins at slots!`7]``", false, pPlayer);
                pWorld->SendPlayPositionedToAll(pPlayer, "slot_win.wav");
                pWorld->SendParticleEffectToAll(PARTICLE_EFFECT_COIN_SPRAY, pTile->GetWorldPosCenter());
            }
            else
            {
                pWorld->SendTalkBubbleAndConsoleToAll(
                    "`7[``" + pPlayer->GetDisplayName(true) + " `4loses at slots!`7]``", false, pPlayer);
                pWorld->SendPlayPositionedToAll(pPlayer, "slot_lose.wav");
            }
        }
        else if (pTileItem->type == ITEM_TYPE_CRYSTAL && pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
        {
        }

        float newTileHealthPercent = pTile->GetHealthPercent();
        if (newTileHealthPercent > 0.0f)
        {
            if ((pTileItem->type == ITEM_TYPE_WEATHER_MACHINE || pTileItem->type == ITEM_TYPE_WEATHER_SPECIAL ||
                 pTileItem->type == ITEM_TYPE_WEATHER_SPECIAL2 ||
                 pTileItem->type == ITEM_TYPE_INFINITY_WEATHER_MACHINE) &&
                tileHealthPercent == 1.0f)
            {
                if (pTileItem->id == ITEM_ID_EPOCH_MACHINE)
                {
                }
                else if (pTileItem->id == ITEM_ID_INFINITY_WEATHER_MACHINE)
                {
                }
                else
                {
                    int32 currentWeather = pWorld->GetCurrentWeather();
                    int32 selectedWeather = pItem->weatherID;

                    if (pWorld->GetCurrentWeather() == pTileItem->weatherID)
                    {
                        pWorld->SetCurrentWeather(WEATHER_TYPE_DEFAULT);
                    }
                    else
                    {
                        pWorld->SetCurrentWeather(pTileItem->weatherID);
                    }

                    pWorld->SendCurrentWeatherToAll();
                }
            }

            if (pTileItem->type == ITEM_TYPE_SUCKER && tileHealthPercent == 1.0f)
            {
                pWorld->GetSuckerBlockManager().TogglePlanting(pPlayer, pTile);
            }
        }
        else
        {
            if (pTileItem->type == ITEM_TYPE_SUCKER)
            {
                if (TileExtra_Sucker* pTileExtra = pTile->GetExtra<TileExtra_Sucker>())
                {
                    if (pTileExtra->count != 0)
                    {
                        pPlayer->SendOnTalkBubble("Empty the " + pTileItem->name + " before breaking!", false);
                        return;
                    }
                }
            }

            if (pTileItem->type == ITEM_TYPE_DRESSUP && !pPlayer->TryWearAllItemsFromDressup(pTile))
                return;

            if (pTileItem->type == ITEM_TYPE_LOCK)
            {
                pWorld->OnRemoveLock(pPlayer, pTile);
            }

            if (pTileItem->type == ITEM_TYPE_XENONITE)
            {
                pWorld->ToggleXenoniteCrystal(true);
            }

            if (pTileItem->type == ITEM_TYPE_DISPLAY_BLOCK)
            {
                if (TileExtra_DisplayBlock* pTileExtra = pTile->GetExtra<TileExtra_DisplayBlock>())
                {
                    if (pTileExtra->itemID != ITEM_ID_BLANK)
                    {
                        ItemInfo* pDispItem = GetItemInfoManager()->GetItemByID(pTileExtra->itemID);
                        if (!pDispItem)
                            return;

                        if (pPlayer->GetInventory().GetFitItemCount(pTileExtra->itemID) < 1)
                        {
                            pWorld->DropObjectOnTile(pTile, pDispItem->id, 1, GetRandomItemDropOffset(), true);
                        }
                        else
                        {
                            pPlayer->ModifyInventoryItem(pDispItem->id, 1);
                        }

                        pTileExtra->itemID = ITEM_ID_BLANK;
                    }
                }
            }

            // tile broken
            if (!pItem->HasFlag(ITEM_FLAG_PERMANENT) && pItem->rarity != 999)
            {
                if (!pTile->IsTree())
                {
                    pPlayer->GiveXP(pItem->rarity / 5 + 1);
                }

                pPlayer->GetProgressData().AddProgress(PLAYER_PROGRESS_BREAK_COUNT, 1);
            }

            if (pTileItem->HasFlag(ITEM_FLAG_AUTOPICKUP))
            {
                pWorld->ThrowItemToPlayerFromPosition(pPlayer, pTile->GetWorldPosCenter(), pTileItem->id, 1);
                pPlayer->ModifyInventoryItem(pTileItem->id, 1);
            }
            else
            {
                pWorld->OnTileDestroyedDropObject(pPlayer, pTile);
            }

            pWorld->HandleTilePackets(pPacket);

            if (pTile->HasFlag(TILE_FLAG_PAINTED_WHITE))
            {
                pTile->RemoveFlag(TILE_FLAG_PAINTED_WHITE);
            }
        }
    }

    // todo here
    if (pItem->id == ITEM_ID_FIST && pTile->GetHealthPercent() > 0.0f)
    {
        pWorld->SendTileApplyDamage(pTile, pPlayer->GetCharData().punchDamage, pPlayer->GetNetID());
    }

    pWorld->SendTileUpdate(pTile);
}