#include "XenoniteDialog.h"
#include "Item/ItemInfoManager.h"
#include "../../World/WorldManager.h"
#include "../GamePlayer.h"
#include "Utils/DialogBuilder.h"

void XenoniteDialog::Request(GamePlayer* pPlayer, TileInfo* pTile, ItemInfo* pItem)
{
    if(!pPlayer || !pTile || !pItem)
        return;

    if(pItem->type != ITEM_TYPE_XENONITE)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    TileExtra_Xenonite* pTileExtra = pTile->GetExtra<TileExtra_Xenonite>();
    if(!pTileExtra)
        return;

    Vector2Int& vTilePos = pTile->GetPos();

    DialogBuilder db;
    db.AddLabelWithIcon(pItem->name, pItem->id, true)
    ->EmbedData("tilex", vTilePos.x)
    ->EmbedData("tiley", vTilePos.y);

    if(!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
    {
        if(pTileExtra->HasFlag(TILE_EXTRA_XENO_F_DOUBLE_JUMP))
            db.AddTextBox("`2Double Jump power is given to all players.``");
        else if(pTileExtra->HasFlag(TILE_EXTRA_XENO_B_DOUBLE_JUMP))
            db.AddTextBox("`6Double Jump power is blocked for all players.``");
        else
            db.AddTextBox("Double Jump power can be used if equipped.");

        if(pTileExtra->HasFlag(TILE_EXTRA_XENO_F_HIGH_JUMP))
            db.AddTextBox("`2High Jump power is given to all players.``");
        else if(pTileExtra->HasFlag(TILE_EXTRA_XENO_B_HIGH_JUMP))
            db.AddTextBox("`6High Jump power is blocked for all players.``");
        else
            db.AddTextBox("High Jump power can be used if equipped.");

        if(pTileExtra->HasFlag2(TILE_EXTRA_XENO_F_HEAT_RESIST))
            db.AddTextBox("`2Heat Resist power is given to all players.``");
        else if(pTileExtra->HasFlag2(TILE_EXTRA_XENO_B_HEAT_RESIST))
            db.AddTextBox("`6Heat Resist power is blocked for all players.``");
        else
            db.AddTextBox("Heat Resist power can be used if equipped.");

        if(pTileExtra->HasFlag(TILE_EXTRA_XENO_F_STRONG_PUNCH))
            db.AddTextBox("`2Strong Punch power is given to all players.``");
        else if(pTileExtra->HasFlag(TILE_EXTRA_XENO_B_STRONG_PUNCH))
            db.AddTextBox("`6Strong Punch power is blocked for all players.``");
        else
            db.AddTextBox("Strong Punch power can be used if equipped.");

        if(pTileExtra->HasFlag2(TILE_EXTRA_XENO_F_LONG_PUNCH))
            db.AddTextBox("`2Long Punch power is given to all players.``");
        else if(pTileExtra->HasFlag2(TILE_EXTRA_XENO_B_LONG_PUNCH))
            db.AddTextBox("`6Long Punch power is blocked for all players.``");
        else
            db.AddTextBox("Long Punch power can be used if equipped.");

        if(pTileExtra->HasFlag(TILE_EXTRA_XENO_F_SPEED))
            db.AddTextBox("`2Speedy power is given to all players.``");
        else if(pTileExtra->HasFlag(TILE_EXTRA_XENO_B_SPEED))
            db.AddTextBox("`6Speedy power is blocked for all players.``");
        else
            db.AddTextBox("Speedy power can be used if equipped.");

        if(pTileExtra->HasFlag2(TILE_EXTRA_XENO_F_LONG_BUILD))
            db.AddTextBox("`2Long Build power is given to all players.``");
        else if(pTileExtra->HasFlag2(TILE_EXTRA_XENO_B_LONG_BUILD))
            db.AddTextBox("`6Long Build power is blocked for all players.``");
        else
            db.AddTextBox("Long Build power can be used if equipped.");

        if(pTileExtra->HasFlag2(TILE_EXTRA_XENO_B_POWERUPS))
            db.AddTextBox("`6Temporary powerups like Balloons and Coffee are not usable.``");

        db.EndDialog("xenonite_edit", "", "OK");
    }
    else
    {
        db.AddTextBox("This crystal can either grant or block super powers for all players in your  world! Any power that\'s unchecked will work as normal - people will have the power if they equip an item with it.")
        ->AddSpacer();

        db.AddCheckBox("checkbox_force_dbl", "Force Double Jump", pTileExtra->HasFlag(TILE_EXTRA_XENO_F_DOUBLE_JUMP))
        ->AddCheckBox("checkbox_block_dbl", "Block Double Jump", pTileExtra->HasFlag(TILE_EXTRA_XENO_B_DOUBLE_JUMP))
        ->AddSpacer();

        db.AddCheckBox("checkbox_force_hig", "Force High Jump", pTileExtra->HasFlag(TILE_EXTRA_XENO_F_HIGH_JUMP))
        ->AddCheckBox("checkbox_block_hig", "Block High Jump", pTileExtra->HasFlag(TILE_EXTRA_XENO_B_HIGH_JUMP))
        ->AddSpacer();
       
        db.AddCheckBox("checkbox_force_asb", "Force Heat Resist", pTileExtra->HasFlag2(TILE_EXTRA_XENO_F_HEAT_RESIST))
        ->AddCheckBox("checkbox_block_asb", "Block Heat Resist", pTileExtra->HasFlag2(TILE_EXTRA_XENO_B_HEAT_RESIST))
        ->AddSpacer();
        
        db.AddCheckBox("checkbox_force_pun", "Force Strong Punch", pTileExtra->HasFlag(TILE_EXTRA_XENO_F_STRONG_PUNCH))
        ->AddCheckBox("checkbox_block_pun", "Block Strong Punch", pTileExtra->HasFlag(TILE_EXTRA_XENO_B_STRONG_PUNCH))
        ->AddSpacer();
        
        db.AddCheckBox("checkbox_force_lng", "Force Long Punch", pTileExtra->HasFlag2(TILE_EXTRA_XENO_F_LONG_PUNCH))
        ->AddCheckBox("checkbox_block_lng", "Block Long Punch", pTileExtra->HasFlag2(TILE_EXTRA_XENO_B_LONG_PUNCH))
        ->AddSpacer();
        
        db.AddCheckBox("checkbox_force_spd", "Force Speedy", pTileExtra->HasFlag(TILE_EXTRA_XENO_F_SPEED))
        ->AddCheckBox("checkbox_block_spd", "Block Speedy", pTileExtra->HasFlag(TILE_EXTRA_XENO_B_SPEED))
        ->AddSpacer();
        
        db.AddCheckBox("checkbox_force_lngb", "Force Long Build", pTileExtra->HasFlag2(TILE_EXTRA_XENO_F_LONG_BUILD))
        ->AddCheckBox("checkbox_block_lngb", "Block Long Build", pTileExtra->HasFlag2(TILE_EXTRA_XENO_B_LONG_BUILD))
        ->AddSpacer();
        
        db.AddCheckBox("checkbox_block_pwr", "Block Use of Powerups", pTileExtra->HasFlag2(TILE_EXTRA_XENO_B_POWERUPS))
        ->AddSpacer();

        db.EndDialog("xenonite_edit", "Update", "Cancel");
    }

    pPlayer->SendOnDialogRequest(db.Get());
}

void XenoniteDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<40>& packet)
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
    if(pTileX->GetInt(tileX) != TO_INT_SUCCESS)
        return;

    int32 tileY = 0;
    if(pTileY->GetInt(tileY) != TO_INT_SUCCESS)
        return;

    TileInfo* pTile = pWorld->GetTileManager()->GetTile(tileX, tileY);
    if(!pTile)
        return;

    TileExtra_Xenonite* pTileExtra = pTile->GetExtra<TileExtra_Xenonite>();
    if(!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Huh? The crystal is gone!", false);
        return;
    }

    if(!pWorld->PlayerHasAccessOnTile(pPlayer, pTile))
    {
        pPlayer->SendOnTalkBubble("No hacking the crystal!", false);
        return;
    }

    pTileExtra->flags = 0;
    pTileExtra->flags2 = 0;

    auto pForceDbl = packet.Find("checkbox_force_dbl"_hash);
    auto pBlockDbl = packet.Find("checkbox_block_dbl"_hash);
    if(pForceDbl || pBlockDbl) {
        bool force = false, block = false;
        if(pForceDbl && pForceDbl->GetBool(force) != TO_INT_SUCCESS) return;
        if(pBlockDbl && pBlockDbl->GetBool(block) != TO_INT_SUCCESS) return;

        force ? pTileExtra->SetFlag(TILE_EXTRA_XENO_F_DOUBLE_JUMP) : pTileExtra->RemoveFlag(TILE_EXTRA_XENO_F_DOUBLE_JUMP);
        block ? pTileExtra->SetFlag(TILE_EXTRA_XENO_B_DOUBLE_JUMP) : pTileExtra->RemoveFlag(TILE_EXTRA_XENO_B_DOUBLE_JUMP);
    }

    auto pForceHig = packet.Find("checkbox_force_hig"_hash);
    auto pBlockHig = packet.Find("checkbox_block_hig"_hash);
    if(pForceHig || pBlockHig) {
        bool force = false, block = false;
        if(pForceHig && pForceHig->GetBool(force) != TO_INT_SUCCESS) return;
        if(pBlockHig && pBlockHig->GetBool(block) != TO_INT_SUCCESS) return;

        force ? pTileExtra->SetFlag(TILE_EXTRA_XENO_F_HIGH_JUMP) : pTileExtra->RemoveFlag(TILE_EXTRA_XENO_F_HIGH_JUMP);
        block ? pTileExtra->SetFlag(TILE_EXTRA_XENO_B_HIGH_JUMP) : pTileExtra->RemoveFlag(TILE_EXTRA_XENO_B_HIGH_JUMP);
    }

    auto pForceAsb = packet.Find("checkbox_force_asb"_hash);
    auto pBlockAsb = packet.Find("checkbox_block_asb"_hash);
    if(pForceAsb || pBlockAsb) {
        bool force = false, block = false;
        if(pForceAsb && pForceAsb->GetBool(force) != TO_INT_SUCCESS) return;
        if(pBlockAsb && pBlockAsb->GetBool(block) != TO_INT_SUCCESS) return;

        force ? pTileExtra->SetFlag2(TILE_EXTRA_XENO_F_HEAT_RESIST) : pTileExtra->RemoveFlag2(TILE_EXTRA_XENO_F_HEAT_RESIST);
        block ? pTileExtra->SetFlag2(TILE_EXTRA_XENO_B_HEAT_RESIST) : pTileExtra->RemoveFlag2(TILE_EXTRA_XENO_B_HEAT_RESIST);
    }

    auto pForcePun = packet.Find("checkbox_force_pun"_hash);
    auto pBlockPun = packet.Find("checkbox_block_pun"_hash);
    if(pForcePun || pBlockPun) {
        bool force = false, block = false;
        if(pForcePun && pForcePun->GetBool(force) != TO_INT_SUCCESS) return;
        if(pBlockPun && pBlockPun->GetBool(block) != TO_INT_SUCCESS) return;

        force ? pTileExtra->SetFlag(TILE_EXTRA_XENO_F_STRONG_PUNCH) : pTileExtra->RemoveFlag(TILE_EXTRA_XENO_F_STRONG_PUNCH);
        block ? pTileExtra->SetFlag(TILE_EXTRA_XENO_B_STRONG_PUNCH) : pTileExtra->RemoveFlag(TILE_EXTRA_XENO_B_STRONG_PUNCH);
    }

    auto pForceLng = packet.Find("checkbox_force_lng"_hash);
    auto pBlockLng = packet.Find("checkbox_block_lng"_hash);
    if(pForceLng || pBlockLng) {
        bool force = false, block = false;
        if(pForceLng && pForceLng->GetBool(force) != TO_INT_SUCCESS) return;
        if(pBlockLng && pBlockLng->GetBool(block) != TO_INT_SUCCESS) return;

        force ? pTileExtra->SetFlag2(TILE_EXTRA_XENO_F_LONG_PUNCH) : pTileExtra->RemoveFlag2(TILE_EXTRA_XENO_F_LONG_PUNCH);
        block ? pTileExtra->SetFlag2(TILE_EXTRA_XENO_B_LONG_PUNCH) : pTileExtra->RemoveFlag2(TILE_EXTRA_XENO_B_LONG_PUNCH);
    }

    auto pForceSpd = packet.Find("checkbox_force_spd"_hash);
    auto pBlockSpd = packet.Find("checkbox_block_spd"_hash);
    if(pForceSpd || pBlockSpd) {
        bool force = false, block = false;
        if(pForceSpd && pForceSpd->GetBool(force) != TO_INT_SUCCESS) return;
        if(pBlockSpd && pBlockSpd->GetBool(block) != TO_INT_SUCCESS) return;

        force ? pTileExtra->SetFlag(TILE_EXTRA_XENO_F_SPEED) : pTileExtra->RemoveFlag(TILE_EXTRA_XENO_F_SPEED);
        block ? pTileExtra->SetFlag(TILE_EXTRA_XENO_B_SPEED) : pTileExtra->RemoveFlag(TILE_EXTRA_XENO_B_SPEED);
    }

    auto pForceLngb = packet.Find("checkbox_force_lngb"_hash);
    auto pBlockLngb = packet.Find("checkbox_block_lngb"_hash);
    if(pForceLngb || pBlockLngb) {
        bool force = false, block = false;
        if(pForceLngb && pForceLngb->GetBool(force) != TO_INT_SUCCESS) return;
        if(pBlockLngb && pBlockLngb->GetBool(block) != TO_INT_SUCCESS) return;

        force ? pTileExtra->SetFlag2(TILE_EXTRA_XENO_F_LONG_BUILD) : pTileExtra->RemoveFlag2(TILE_EXTRA_XENO_F_LONG_BUILD);
        block ? pTileExtra->SetFlag2(TILE_EXTRA_XENO_B_LONG_BUILD) : pTileExtra->RemoveFlag2(TILE_EXTRA_XENO_B_LONG_BUILD);
    }

    if(auto pBlockPwr = packet.Find("checkbox_block_pwr"_hash)) {
        bool block = false;
        if(pBlockPwr->GetBool(block) != TO_INT_SUCCESS) return;

        block ? pTileExtra->SetFlag2(TILE_EXTRA_XENO_B_POWERUPS) : pTileExtra->RemoveFlag2(TILE_EXTRA_XENO_B_POWERUPS);
    }

    pWorld->SendConsoleMessageToAll("The Xenonite Crystal has shifted...");
    pWorld->ToggleXenoniteCrystal(true);
}
