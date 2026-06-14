#include "OuijaBoardDialog.h"
#include "Item/ItemInfoManager.h"
#include "../../World/WorldManager.h"
#include "../GamePlayer.h"
#include "Utils/DialogBuilder.h"

void OuijaBoardDialog::RequestMain(GamePlayer* pPlayer, TileInfo* pTile)
{
    if(!pPlayer || !pTile)
        return;

    TileExtra_OuijaBoard* pTileExtra = pTile->GetExtra<TileExtra_OuijaBoard>();
    if(!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Woops, its gone.", false);
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if(!pItem)
        return;

    if(pItem->type != ITEM_TYPE_OUIJA_BOARD)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    bool isDarkSpiritBoard = pTile->GetFG() == ITEM_ID_DARK_SPIRIT_BOARD;

    if(pTileExtra->items.empty() && !pWorld->GetTileManager()->RandomizeOuijaBoardTile(pTile))
    {
        pPlayer->SendOnTalkBubble("Opps, something happened badly.", false);
        LOGGER_LOG_WARN("Tried to generate ouija but it failed!? isDarkSpirit: %d, player: %d world: %d", isDarkSpiritBoard, pPlayer->GetNetID(), pWorld->GetInstanceID());
        return;
    }

    DialogBuilder db;
    db.SetDefaultColor('o')
    ->AddLabelWithIcon("`w" + pItem->name, pItem->id, true)
    ->AddLabel("You sense a presence in the room...");

#ifndef _DEBUG
    RectFloat tileRect = pTile->GetRect();
    tileRect.InFlate(32 * 2);

    if(pWorld->GetPlayersInWorldRect(tileRect).size() < 2)
    {
        db.AddTextBox("The planchette wobbles and then remains still...")
        ->AddTextBox("Perhaps you need more people to help.")
        ->EndDialog("ouijaboard", "", "Close");
        pPlayer->SendOnDialogRequest(db.Get());
        return;
    }
#endif

    int32 ouijaType = 0;
    if(ToInt(pTileExtra->ouijaType, ouijaType) != TO_INT_SUCCESS)
        return;

    db.AddTextBox("The planchette slowly comes to life and begins to drift across the board...")
    ->AddSpacer()
    ->AddTextBox("Ask your questions")
    ->AddButton("showOuijaBoardItems", "What do you require of us?")
    ->AddButton("showOuijaBoardCommand", "How can we bring you here?");

    if(ouijaType == 1)
    {
        db.AddSpacer()
        ->AddSmallText("A small inscription is etched into the bottom of the board!")
        ->AddSmallText("`4HE IS TOO POWERFUL! HE WILL DESTROY YOUR WORLD!``");
    }

    Vector2Int& vTilePos = pTile->GetPos();

    db.AddButton("adminTrigger", "`4Trigger");

    db.AddSpacer()
    ->EmbedData("tilex", vTilePos.x)
    ->EmbedData("tiley", vTilePos.y)
    ->EndDialog("ouijaboard", "", "Close");

    pPlayer->SendOnDialogRequest(db.Get());
}

void OuijaBoardDialog::RequestItemInfo(GamePlayer* pPlayer, TileInfo* pTile, int32 itemIndex, eOuijaItemInfoType type)
{
    if(!pPlayer || !pTile || itemIndex < -1 || itemIndex >= 2)
        return;

    TileExtra_OuijaBoard* pTileExtra = pTile->GetExtra<TileExtra_OuijaBoard>();
    if(!pTileExtra)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    if(pTileExtra->items.empty() && !pWorld->GetTileManager()->RandomizeOuijaBoardTile(pTile))
    {
        pPlayer->SendOnTalkBubble("Opps, something happened badly.", false);
        return;
    }

    if(pTileExtra->items.size() % 3 != 0 || pTileExtra->items.size() / 3 < 1 || (itemIndex != -1 && (itemIndex + 1 > pTileExtra->items.size() / 3)))
        return;

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if(!pItem)
        return;

    if(pItem->type != ITEM_TYPE_OUIJA_BOARD)
        return;

    Vector2Int& vTilePos = pTile->GetPos();
    
    DialogBuilder db;
    db.SetDefaultColor('o')
    ->AddLabelWithIcon("`w" + pItem->name, pItem->id, true)
    ->EmbedData("tilex", vTilePos.x)
    ->EmbedData("tiley", vTilePos.y);

#ifndef _DEBUG
    RectFloat tileRect = pTile->GetRect();
    tileRect.InFlate(32 * 2);

    if(pWorld->GetPlayersInWorldRect(tileRect).size() < 2)
    {
        db.AddTextBox("You sense a presence in the room...")
        ->AddTextBox("The planchette wobbles and then remains still...")
        ->AddTextBox("Perhaps you need more people to help.")
        ->EndDialog("ouijaboard", "", "Close");
        pPlayer->SendOnDialogRequest(db.Get());
        return;
    }
#endif

    if(itemIndex == -1)
    {
        if(pTileExtra->items.size() / 2 == 1)
        {
            db.AddTextBox("I need a specific item to be worn to build up enough energy for me to come over.")
            ->AddButton("showOuijaBoardItems1", "What item do we need?");
        }
        else
        {
            db.AddTextBox("I need 2 specific items to be worn to build up enough energy for me to come over.")
            ->AddButton("showOuijaBoardItems1", "What is the first item?")
            ->AddButton("showOuijaBoardItems2", "What is the second item?");
        }

        db.AddSpacer()
        ->AddButton("backToOuijaBoard", "Back")
        ->EndDialog("ouijaboard", "", "Goodbye");
        pPlayer->SendOnDialogRequest(db.Get());
        return;
    }
    
    ItemInfo* pOuijaItem = GetItemInfoManager()->GetItemByID(pTileExtra->items[itemIndex * 2 + 1]);
    if(!pItem)
        return;

    string itemButtonID = ToString(itemIndex + 1);
    
    switch(type)
    {
        case eOuijaItemInfoType::MAIN:
        {
            db.AddTextBox("I can't remember exactly what item is it. Maybe you can help jog my memory...")
            ->AddSpacer()
            ->AddButton("showOuijaBoardItems" + itemButtonID + "name", "What is the item's name?")
            ->AddButton("showOuijaBoardItems" + itemButtonID + "amount", "How many do you need?")
            ->AddButton("showOuijaBoardItems" + itemButtonID + "rarity", "What rarity is it?")
            ->AddButton("showOuijaBoardItems" + itemButtonID + "seed", "How is it made?")
            ->AddSpacer()
            ->AddButton("showOuijaBoardItems", "Back");
            break;
        }

        case eOuijaItemInfoType::NAME:
        {
            if(pOuijaItem->name.empty())
                return;

            string nameBox = "I remember that it's name starts with `5\"";
            nameBox += pOuijaItem->name[0]; 
            nameBox += "\"``.";

            db.AddTextBox(nameBox);
            break;
        }

        case eOuijaItemInfoType::AMOUNT:
        {
            db.AddTextBox("To generate enough energy, `5" + ToString(pTileExtra->items[itemIndex * 2 + 2]) + "`` of you need to be wearing it.");
            break;
        }

        case eOuijaItemInfoType::RARITY:
        {
            db.AddTextBox("I remember that it has a rarity of `5" + ToString(pOuijaItem->rarity) + "``.");
            break;
        }

        case eOuijaItemInfoType::SEED:
        {
            if(pOuijaItem->seed1 == 0 && pOuijaItem->seed2 == 0)
            {
                db.AddTextBox("I remember that it can't be spliced.");
                break;
            }

            ItemInfo* pSeed = GetItemInfoManager()->GetItemByID(pOuijaItem->seed1);
            if(!pSeed)
            {
                db.AddTextBox("I lost mind...");
                break;
            }

            db.AddTextBox("I remember that it can be grown by splicing `5" + pSeed->name + "`` with something.");
            break;
        }
    }

    if(type != eOuijaItemInfoType::MAIN)
    {
        db.AddSpacer()
        ->AddButton("showOuijaBoardItems" + itemButtonID, "Back");
    }

    db.EndDialog("ouijaboard", "", "Goodbye");
    pPlayer->SendOnDialogRequest(db.Get());
}

void OuijaBoardDialog::RequestCommand(GamePlayer* pPlayer, TileInfo* pTile)
{
    if(!pPlayer || !pTile)
        return;

    TileExtra_OuijaBoard* pTileExtra = pTile->GetExtra<TileExtra_OuijaBoard>();
    if(!pTileExtra)
    {
        pPlayer->SendOnTalkBubble("Woops, its gone.", false);
        return;
    }

    ItemInfo* pItem = GetItemInfoManager()->GetItemByID(pTile->GetFG());
    if(!pItem)
        return;

    if(pItem->type != ITEM_TYPE_OUIJA_BOARD)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    if(pTileExtra->items.empty() && !pWorld->GetTileManager()->RandomizeOuijaBoardTile(pTile))
    {
        pPlayer->SendOnTalkBubble("Opps, something happened badly.", false);
        return;
    }

    DialogBuilder db;
    db.SetDefaultColor('o')
    ->AddLabelWithIcon("`w" + pItem->name, pItem->id, true)
    ->AddTextBox("The mood in the room needs to match mine...")
    ->AddTextBox("...");

#ifndef _DEBUG
    RectFloat tileRect = pTile->GetRect();
    tileRect.InFlate(32 * 2);

    if(pWorld->GetPlayersInWorldRect(tileRect).size() < pTileExtra->playerCount)
    {
        db.AddTextBox("But there arn't enough people around.")
        ->EndDialog("ouijaboard", "", "Close");
        pPlayer->SendOnDialogRequest(db.Get());
        return;
    }
#endif

    static std::unordered_map<string, string> commandLabel =
    {
        {"/cry", "and I'm so sad I could cry..."},
        {"/furious", "and I'm so mad I could scream..."},
        {"/rolleyes", "and you're unbelievable. I mean, really?! Come on..."},
        {"/omg", "and I'm SOOOOOOOoooo hyped! ..."},
        {"/wave", "and I feel like greeting someone..."},
        {"/dance", "and I just want to party..."},
        {"/love", "and I feel the love.."},
        {"/sleep", "and I'm so sleepy..."},
        {"/fp", "and I can't believe what just happened. I mean,  WHY?..."}
    };

    auto it = commandLabel.find(pTileExtra->command);
    if(it != commandLabel.end())
    {
        db.AddLabel(it->second);
    }
    else
    {
        db.AddLabel("Woah... I just lost my mood");
    }

    db.AddSpacer()
    ->AddButton("backToOuijaBoard", "Back")
    ->EndDialog("ouijaboard", "", "Close");
    pPlayer->SendOnDialogRequest(db.Get());
}

void OuijaBoardDialog::Handle(GamePlayer* pPlayer, ParsedTextPacket<40>& packet)
{
    if(!pPlayer)
        return;

    auto pButtonClicked = packet.Find("buttonClicked"_hash);
    if(!pButtonClicked || (pButtonClicked && pButtonClicked->size == 0))
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

    TileExtra_OuijaBoard* pTileExtra = pTile->GetExtra<TileExtra_OuijaBoard>();
    if(!pTileExtra)
        return;

    uint32 clickedButtonHash = HashString(pButtonClicked->value, pButtonClicked->size);

    switch(clickedButtonHash)
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
            pWorld->TriggerOuijaBoard({ pPlayer }, pTile);
            return;
        }
    }
}
