#include "../Dialog/GameDialogs.h"
#include "../Player/PlayerManager.h"
#include "../Server/MasterBroadway.h"
#include "../World/WorldManager.h"
#include "CommandManager.h"
#include "Item/ItemInfoManager.h"
#include "Math/Math.h"
#include "Math/Random.h"
#include "Utils/StringUtils.h"

CommandManager* GetCommandManager()
{
    return CommandManager::GetInstance();
}

MAKE_COMMAND(AgeWorld, "/ageworld <ageMin>", "Age current map", "command.ageworld"_hash, "ageworld"_hash)
{
    if (args.size() < 2)
    {
        pPlayer->SendOnConsoleMessage("`oUsage: " + GetInfo().usage);
        return;
    }

    uint32 ageMin = 0;
    if (ToUInt(args[1], ageMin) != TO_INT_SUCCESS)
    {
        pPlayer->SendOnConsoleMessage("`oAgeMin must be positive number.");
        return;
    }

    if (ageMin > 100000)
    {
        pPlayer->SendOnConsoleMessage("`oUmm, the target age seems like so old... please age it a bit lesser");
        return;
    }

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    pWorld->GetTileManager()->AgeTiles(ageMin * 60 * 1000);
    pPlayer->SendOnConsoleMessage("World \"`#" + pWorld->GetWorlName() + "``\" aged " + ToString(ageMin) + " minutes.");
    pWorld->ReconnectPlayers();
}

MAKE_COMMAND(Emotes, "", "", 0, "dance"_hash, "dance2"_hash, "facepalm"_hash, "mad"_hash, "shrug"_hash, "foldarms"_hash,
             "stubborn"_hash, "fold"_hash, "rolleyes"_hash, "eyeroll"_hash, "sad"_hash, "fa"_hash, "idk"_hash,
             "no"_hash, "omg"_hash, "yes"_hash, "sleep"_hash, "cheer"_hash, "troll"_hash, "love"_hash, "kiss"_hash,
             "wave"_hash, "fp"_hash)
{
    if (pPlayer->GetLastActionTime().GetElapsedTime() <= 200)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    pPlayer->GetLastActionTime().Reset();
    pWorld->SendOnActionToAll(pPlayer, args[0]);
    pWorld->CheckOuijaBoardCommand(pPlayer, args[0]);
}

MAKE_COMMAND(FindItem, "/finditem <item_name>", "Find item from database", "command.finditem"_hash, "finditem"_hash)
{
    if (args.size() < 2)
    {
        pPlayer->SendOnConsoleMessage("`oUsage: " + GetInfo().usage);
        return;
    }

    string itemName = JoinString(args, " ", 1);
    StripWhiteSpace(itemName);

    if (itemName.size() < 3)
    {
        pPlayer->SendOnConsoleMessage("`4Error:`` Item name must be at least `w3`` characters long.");
        return;
    }

    auto items = GetItemInfoManager()->GetSortedItemIdsByName(itemName, true);
    if (items.empty())
    {
        pPlayer->SendOnConsoleMessage("`4Error:`` No items matched your search criteria.");
        return;
    }

    if (items.size() > 5000)
    {
        pPlayer->SendOnConsoleMessage("`4Error:`` Too many results found. Please provide a more specific item name.");
        return;
    }

    pPlayer->OpenPaginatedDialog(std::make_unique<ItemFinderDialog>(std::move(items)));
}

MAKE_COMMAND(Ghost, "/ghost", "Walk throught blocks", "command.ghost"_hash, "ghost"_hash)
{
    PlayerPlayModController& modController = pPlayer->GetModController();

    if (modController.HasPlayMod(PLAYMOD_TYPE_GHOST))
    {
        modController.RemovePlayMod(PLAYMOD_TYPE_GHOST);
    }
    else
    {
        modController.AddPlayMod(PLAYMOD_TYPE_GHOST);
    }
}

MAKE_COMMAND(GiveItem, "/giveitem <userID> <amount> <item name>", "Give item to player", "command.giveitem"_hash,
             "giveitem"_hash)
{
    if (args.size() < 4)
    {
        pPlayer->SendOnConsoleMessage("`oUsage: " + GetInfo().usage);
        return;
    }

    uint32 userID = 0;
    if (ToUInt(args[1], userID) != TO_INT_SUCCESS)
    {
        pPlayer->SendOnConsoleMessage("`oUserID must be number!");
        return;
    }

    uint32 amount = 0;
    if (ToUInt(args[2], amount) != TO_INT_SUCCESS)
    {
        pPlayer->SendOnConsoleMessage("`oItem amount must be number!");
        return;
    }

    string itemName = JoinString(args, " ", 3);
    ItemInfo* pItem = GetItemInfoManager()->GetItemByName(itemName);
    if (!pItem)
    {
        pPlayer->SendOnConsoleMessage("`oFailed to find the item " + itemName);
        return;
    }

    GamePlayer* pTarget = GetPlayerManager()->GetPlayerByUserID(userID);
    if (!pTarget)
    {
        pPlayer->SendOnConsoleMessage("`oFailed to find user with given id");
        return;
    }

    uint8 givenCount = pTarget->GetInventory().AddItem(pItem->id, amount, pTarget);
    if (givenCount == 0)
    {
        /**
         * handle
         */
    }

    pPlayer->SendOnConsoleMessage("`oGiven " + ToString(givenCount) + " " + pItem->name + " to " +
                                  pTarget->GetRawName() + " (ID: " + ToString(pTarget->GetUserID()) + ")");
    pTarget->SendOnConsoleMessage("`oGiven: " + ToString(givenCount) + " " + pItem->name);
}

MAKE_COMMAND(Magic, "/magic", "Bass da da da", "command.magic"_hash, "magic"_hash)
{
    if (pPlayer->GetCurrentWorld() == 0)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    pWorld->PlaySFXForEveryone("magic.wav");
    Vector2Float playerPos = pPlayer->GetWorldPos();

    for (uint8 i = 0; i < 20; ++i)
    {
        float offsetX = RandomRangeFloat(-80.0f, 100.0f);
        float offsetY = RandomRangeFloat(-80.0f, 100.0f);

        int32 particleType = RandomRangeInt(0, 3);
        pWorld->SendParticleEffectToAll(playerPos.x + offsetX, playerPos.y + offsetY, particleType, 4, 150 * i);
    }
}

MAKE_COMMAND(RenderWorld, "/renderworld", "View world as image", "command.renderworld"_hash, "renderworld"_hash)
{
    if (pPlayer->GetCurrentWorld() == 0)
        return;

    if (pPlayer->HasState(PLAYER_STATE_RENDERING_WORLD))
    {
        pPlayer->SendOnConsoleMessage("`4OOPS! `oYou already requested for world rendering, you should wait!");
        return;
    }

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    TileInfo* pLockTile = pWorld->GetTileManager()->GetKeyTile(KEY_TILE_WORLD_LOCK);
    if (!pLockTile)
    {
        pPlayer->SendOnTalkBubble("``Sorry, only `5World Locked`` worlds that you own can be rendered.", false);
        return;
    }

    TileExtra_Lock* pTileExtra = pLockTile->GetExtra<TileExtra_Lock>();
    if (pTileExtra->ownerID != pPlayer->GetUserID())
    {
        pPlayer->SendOnTalkBubble("`oSorry, only the world owner can render it.", false);
        return;
    }

    RenderWorldDialog::Request(pPlayer);
}

MAKE_COMMAND(TogglePlayMod, "/toggleplaymod <playmodID>", "Toggle playmods", "command.toggleplaymod"_hash,
             "toggleplaymod"_hash)
{
    if (args.size() < 2)
    {
        pPlayer->SendOnConsoleMessage("`oUsage: " + GetInfo().usage);
        return;
    }

    uint32 playModID = 0;
    if (ToUInt(args[1], playModID) != TO_INT_SUCCESS)
    {
        pPlayer->SendOnConsoleMessage("`PlayModID must be number!");
        return;
    }

    PlayerPlayModController& modController = pPlayer->GetModController();
    if (modController.HasPlayMod((ePlayModType)playModID))
    {
        modController.RemovePlayMod((ePlayModType)playModID);
    }
    else
    {
        modController.AddPlayMod((ePlayModType)playModID);
    }
}

MAKE_COMMAND(Trade, "/trade <playerName>", "Start trade with someone", 0, "trade"_hash)
{
    if (args.size() < 2)
    {
        pPlayer->SendOnConsoleMessage("`oUsage: " + GetInfo().usage);
        return;
    }

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    if (pWorld->GetPlayerCount() <= 1)
    {
        pPlayer->SendOnConsoleMessage("There are no people to trade with.");
        return;
    }

    string error;
    GamePlayer* pTarget = pWorld->GetPlayerByNameStartsWith(args[1], error);
    if (!pTarget)
    {
        pPlayer->SendOnConsoleMessage(error);
        return;
    }

    if (pTarget->GetTradeManager().IsTrading() && pTarget->GetTradeManager().GetPartner() != pPlayer)
    {
        pPlayer->SendOnTalkBubble("That person is busy", false);
        return;
    }

    pPlayer->SendOnStartTrade(pTarget->GetDisplayName(true), pTarget->GetNetID());
}

/////////////////////////////////////////////////////////////

void CommandManager::RegisterAllCommands()
{
    Register<Command_AgeWorld>();
    Register<Command_Emotes>();
    Register<Command_FindItem>();
    Register<Command_Ghost>();
    Register<Command_GiveItem>();
    Register<Command_Magic>();
    Register<Command_RenderWorld>();
    Register<Command_TogglePlayMod>();
    Register<Command_Trade>();
}