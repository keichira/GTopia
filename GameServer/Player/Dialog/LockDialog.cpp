#include "LockDialog.h"
#include "../GamePlayer.h"
#include "Utils/DialogBuilder.h"
#include "Item/ItemInfoManager.h"
#include "../../World/WorldManager.h"
#include "../../Server/UserCacheManager.h"
#include "../../Player/PlayerManager.h"

void LockDialog::Request(GamePlayer* pPlayer, TileInfo* pTile)
{
    if(!pPlayer || !pTile || pPlayer->GetCurrentWorld() == 0)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    TileExtra_Lock* pTileExtra = pTile->GetExtra<TileExtra_Lock>();
    if(!pTileExtra)
        return;

    if(!pTileExtra->HasAccess(pPlayer->GetUserID()))
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetDisplayedItem());
    if(!pItem || pItem->type != ITEM_TYPE_LOCK)
        return;

    if(pTileExtra->ownerID != pPlayer->GetUserID())
    {
        if(pTileExtra->IsAdmin(pPlayer->GetUserID()))
        {
            // not "someone" but lets keep it no need to care much it no?
            DialogBuilder db;
            db.SetDefaultColor('o')
            ->AddLabelWithIcon("`wEdit " + pItem->name, pItem->id, true)
            ->EmbedData("tilex", pTile->GetPos().x)
            ->EmbedData("tiley", pTile->GetPos().y)
            ->AddLabel("This lock is owned by someone, but I have access on it.")
            ->EndDialog("lock_edit", "Remove My Access", "Cancel");
            pPlayer->SendOnDialogRequest(db.Get());
        }
        else
        {
            pPlayer->SendOnTalkBubble("I'm `4unable`` to pick the lock.", true);
        }

        return;
    }

    Vector2Int& vTilePos = pTile->GetPos();

    if(pTileExtra->GetTotalAccessedCount() == 0)
    {
        HandleFromCache(pPlayer, pWorld->GetInstanceID(), vTilePos.x, vTilePos.y);
    }
    else
    {
        GetUserCacheManager()->FetchMetadata(
            pPlayer->GetNetID(),
            CACHE_REQ_WORLD_LOCK_DIALOG,
            pTileExtra->accessList,
            { pWorld->GetInstanceID(), vTilePos.x, vTilePos.y }
        );
    }
}

void LockDialog::HandleFromCache(GamePlayer* pPlayer, uint32 worldInstanceID, int32 tileX, int32 tileY)
{
    if(!pPlayer || pPlayer->GetCurrentWorld() != worldInstanceID)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(worldInstanceID);
    if(!pWorld)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if(!pTile)
        return;

    TileExtra_Lock* pTileExtra = pTile->GetExtra<TileExtra_Lock>();
    if(!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("I'm `4unable`` to pick the lock.", true);
        return;
    }

    if(pTileExtra->ownerID != pPlayer->GetUserID())
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if(pItem->type != ITEM_TYPE_LOCK)
        return;

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.SetDefaultColor('o')
    ->AddLabelWithIcon("`wEdit " + pItem->name, pItem->id, true)
    ->EmbedData("tilex", vTilePos.x)
    ->EmbedData("tiley", vTilePos.y)
    ->AddLabel("`wAccess list:``")
    ->AddSpacer();

    uint32 totalAcccessedCount = pTileExtra->GetTotalAccessedCount();
    if(totalAcccessedCount > 0)
    {
        UserCacheManager* pUserMgr = GetUserCacheManager();

        for(auto& id : pTileExtra->accessList)
        {
            if(id > 0 && id != pPlayer->GetUserID())
            {
                UserMetadata* pMetaData = pUserMgr->GetMetadata(id);
                if(!pMetaData)
                {
                    db.AddCheckBox("checkbox_" + ToString(id), "Error get name (#" + ToString(id) + ")", true);
                    continue;
                }

                db.AddCheckBox("checkbox_" + ToString(id), pMetaData->name, true);
            }
        }   
    }
    else
    {
        db.AddLabel("Currently. you're the only one with access.``");
    }

    if(totalAcccessedCount < 26)
    {
        db.AddSpacer()
        ->AddPlayerPicker("playerNetID", "`wAdd``");
    }
    else
    {
        db.AddSpacer()
        ->AddLabel("`4(max players added)``");
    }

    if(pItem->id == ITEM_ID_BUILDERS_LOCK)
    {
        db.AddCheckBox("checkbox_public", "Allow anyone to Build or Break", pTile->HasFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC));
    }
    else
    {
        db.AddCheckBox("checkbox_public", "Allow anyone to build", pTile->HasFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC));
    }

    if(IsWorldLock(pItem->id))
    {
        db.AddCheckBox("checkbox_disable_music", "Disable Custom Music Blocks", pTileExtra->HasFlag(TILE_EXTRA_LOCK_DISABLE_MUSIC));

        if(!pTileExtra->HasFlag(TILE_EXTRA_LOCK_DISABLE_MUSIC))
        {
            // tempo
        }

        db.AddCheckBox("checkbox_disable_music_render", "Make Custom Music Blocks invisible", pTileExtra->HasFlag(TILE_EXTRA_LOCK_DISABLE_RENDER_MUSIC));

        if(pItem->id == ITEM_ID_ROYAL_LOCK)
        {
            db.AddTextBox("Ye Royal Options")
            ->AddCheckBox("checkbox_silence", "Silence, Peasants!", pTileExtra->HasFlag(TILE_EXTRA_LOCK_SILENCE))
            ->AddCheckBox("checkbox_rainbow", "Royal Rainbows!", pTileExtra->HasFlag(TILE_EXTRA_LOCK_RAINBOW_TRAIL));
        }

        if(pPlayer->GetInventory().GetCountOfItem(ITEM_ID_WORLD_KEY) == 0)
        {
            db.AddButton("getKey", "Get World Key");
        }
    }
    else
    {
        db.AddCheckBox("checkbox_ignore", "Ignore empty air", pTileExtra->HasFlag(TILE_EXTRA_LOCK_IGNORE_EMPTY))
        ->AddButton("recalcLock", "`wRe-apply lock``");

        if(pItem->id == ITEM_ID_BUILDERS_LOCK)
        {
            db.AddSpacer()
            ->AddSmallText("This lock allows Building or Breaking.")
            ->AddSmallText("(ONLY if \"Allow anyone to Build or Break\" is checked above)!")
            ->AddSpacer()
            ->AddSmallText("Leaving this box unchecked only allows Breaking.")
            ->AddCheckBox("checkbox_buildonly", "Only Allow Building!", pTileExtra->HasFlag(TILE_EXTRA_LOCK_BUILD_ONLY))
            ->AddSmallText("People with lock access can both build and break unless you check below. The lock owner can always build and break.")
            ->AddCheckBox("checkbox_admins", "Admins Are Limited", pTileExtra->HasFlag(TILE_EXTRA_LOCK_LIMIT_ADMINS));
        }
    }

    db.EndDialog("lock_edit", "OK", "Cancel");
    pPlayer->SendOnDialogRequest(db.Get());
}

void LockDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<40>& packet)
{
    if(pPlayer->GetCurrentWorld() == 0)
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
    if(pTileX->GetInt(tileX) != TO_INT_SUCCESS)
        return;

    int32 tileY = 0;
    if(pTileY->GetInt(tileY) != TO_INT_SUCCESS)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if(!pTile) 
    {
        pPlayer->SendOnTalkBubble("I was looking at a lock but now it's gone.  Magic is real!", false);
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if(pItem->type != ITEM_TYPE_LOCK)
        return;

    TileExtra_Lock* pTileExtra = pTile->GetExtra<TileExtra_Lock>();
    if(!pTileExtra)
        return;

    if(!pTileExtra->HasAccess(pPlayer->GetUserID()))
        return;

    if(pTileExtra->ownerID != pPlayer->GetUserID()) 
    {
        pTileExtra->RemoveFromList(pPlayer->GetUserID());
        pWorld->SendTileUpdate(pTile, pPlayer);
        pWorld->SendNameChangeToAll(pPlayer);
        pPlayer->PlaySFX("dialog_cancel.wav");

        pWorld->SendConsoleMessageToAll(pPlayer->GetRawName() + "`` removed their access from a " + pItem->name);
        return;
    }

    bool tileNeedsUpdate = false;

    if(auto pIgnoreAir = packet.Find("checkbox_ignore"_hash))
    {
        bool val;
        if(pIgnoreAir->GetBool(val) != TO_INT_SUCCESS) 
            return;
        
        val ? pTileExtra->SetFlag(TILE_EXTRA_LOCK_IGNORE_EMPTY) 
            : pTileExtra->RemoveFlag(TILE_EXTRA_LOCK_IGNORE_EMPTY);
    }

    bool oldPublicFlag = pTile->HasFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC);

    if(auto pPublic = packet.Find("checkbox_public"_hash))
    {
        bool val;
        if(pPublic->GetBool(val) != TO_INT_SUCCESS) 
            return;
        
        val ? pTile->SetFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC) 
            : pTile->RemoveFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC);
    }

    if(pItem->id == ITEM_ID_BUILDERS_LOCK)
    {
        if(auto pBuildOnly = packet.Find("checkbox_buildonly"_hash))
        {
            bool val;
            if(pBuildOnly->GetBool(val) != TO_INT_SUCCESS) 
                return;
            
            val ? pTileExtra->SetFlag(TILE_EXTRA_LOCK_BUILD_ONLY) 
                : pTileExtra->RemoveFlag(TILE_EXTRA_LOCK_BUILD_ONLY);
        }

        if(auto pLimitAdmins = packet.Find("checkbox_admins"_hash))
        {
            bool val;
            if(pLimitAdmins->GetBool(val) != TO_INT_SUCCESS) 
                return;
            
            val ? pTileExtra->SetFlag(TILE_EXTRA_LOCK_LIMIT_ADMINS) 
                : pTileExtra->RemoveFlag(TILE_EXTRA_LOCK_LIMIT_ADMINS);
        }
    }

    if(oldPublicFlag != pTile->HasFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC) && IsWorldLock(pItem->id))
    {
        string notifyPublic = pPlayer->GetDisplayName(true);
        notifyPublic += " `whas set `$World Lock`w to ";
        notifyPublic += pTile->HasFlag(TILE_FLAG_IS_OPEN_TO_PUBLIC) ? "`$PUBLIC" : "`4PRIVATE";
        pWorld->SendConsoleMessageToAll(notifyPublic);
        tileNeedsUpdate = true;
    }

    if(IsWorldLock(pItem->id)) 
    {
        if(auto pDisableMusic = packet.Find("checkbox_disable_music"_hash))
        {
            bool val;
            if(pDisableMusic->GetBool(val) != TO_INT_SUCCESS) 
                return;
            
            val ? pTileExtra->SetFlag(TILE_EXTRA_LOCK_DISABLE_MUSIC) 
                : pTileExtra->RemoveFlag(TILE_EXTRA_LOCK_DISABLE_MUSIC);
        }

        if(pItem->id == ITEM_ID_ROYAL_LOCK)
        {
            if(auto pSilenced = packet.Find("checkbox_silence"_hash))
            {
                bool val;
                if(pSilenced->GetBool(val) != TO_INT_SUCCESS) 
                    return;
        
                if(val)
                {
                    pTileExtra->SetFlag(TILE_EXTRA_LOCK_SILENCE);
                    pWorld->SendTalkBubbleAndConsoleToAll(
                        "`w" + pPlayer->GetDisplayName(true) + "```w has silenced the peasants!``", true
                    );
                }
                else 
                {
                    pTileExtra->RemoveFlag(TILE_EXTRA_LOCK_SILENCE);
                    pWorld->SendTalkBubbleAndConsoleToAll(
                        "`w" + pPlayer->GetDisplayName(true) + "```w has allowed the peasants to speak.``", true
                    );
                }
            }

            if(auto pTrail = packet.Find("checkbox_rainbow"_hash))
            {
                bool val;
                if(pTrail->GetBool(val) != TO_INT_SUCCESS) 
                    return;
        
                val ? pTileExtra->SetFlag(TILE_EXTRA_LOCK_RAINBOW_TRAIL) 
                    : pTileExtra->RemoveFlag(TILE_EXTRA_LOCK_RAINBOW_TRAIL);
            }

            tileNeedsUpdate = true;
        }
    }
   
    if(tileNeedsUpdate)
    {
        pWorld->SendTileUpdate(pTile);
    }

    if(auto pPlayerNetID = packet.Find("playerNetID"_hash))
    {
        uint32 targetPlayer = 0;
        if(pPlayerNetID->GetUInt(targetPlayer) != TO_INT_SUCCESS)
            return;

        GamePlayer* pTarget = GetPlayerManager()->GetPlayerByNetID(targetPlayer);
        if(!pTarget)
        {
            pPlayer->SendOnTalkBubble("Unable to add person to lock. Try again.", false);
            return;
        }

        if(pTileExtra->HasAccess(targetPlayer))
        {
            pPlayer->SendOnTalkBubble(pTarget->GetRawName() + " already has access to the lock", false);
            return;
        }

        if(pTileExtra->GetTotalAccessedCount() > 25)
        {
            pPlayer->SendOnTalkBubble("Unable to add, the lock is full!", false);
            return;
        }

        // todo send acc to player
        return;
    }

    auto pButtonClicked = packet.Find("buttonClicked"_hash);
    if(!pButtonClicked)
        return;

    if(pButtonClicked->size == 0 || pButtonClicked->size > 20)
        return;

    std::string_view clickedButton = pButtonClicked->GetStringView();

    if(clickedButton == "recalcLock" && !IsWorldLock(pItem->id))
    {
        std::vector<TileInfo*> lockedTiles;
        bool lockSuccsess = pWorld->GetTileManager()->ApplyLockTiles(pTile, GetMaxTilesToLock(pItem->id), pTileExtra->HasFlag(TILE_EXTRA_LOCK_IGNORE_EMPTY), lockedTiles);

        if(!lockSuccsess) 
        {
            pPlayer->SendOnTalkBubble("Something went wrong, unable to re-calc lock.", true);
            return;
        }
        else
        {
            pWorld->SendLockPacketToAll(pPlayer->GetUserID(), pItem->id, lockedTiles, pTile);
        }
    }

    if(clickedButton == "getKey" && IsWorldLock(pItem->id))
    {

        //add floating & untradeable check

        if(pTileExtra->GetTotalAccessedCount() != 0)
        {
            pPlayer->SendOnTalkBubble("`4You'll first need to remove all co-owners`` from your `5World Lock`` to get a `#World Key`` to trade this world.", false);
        }
        else
        {
            PlayerInventory& inventory = pPlayer->GetInventory();
            if(inventory.GetCountOfItem(ITEM_ID_WORLD_KEY) != 0)
            {
                pPlayer->SendOnTalkBubble("`4Looks like you already have the key!", false);
                return;
            }

            pPlayer->SendOnTalkBubble("You got a `#World Key``! You can now trade this world to other players.", false);
            pPlayer->ModifyInventoryItem(ITEM_ID_WORLD_KEY, 1);
            pPlayer->PlaySFX("use_lock.wav");
        }
    }
}
