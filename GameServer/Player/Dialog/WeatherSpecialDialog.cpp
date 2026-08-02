#include "WeatherSpecialDialog.h"
#include "../../World/WorldManager.h"
#include "../GamePlayer.h"
#include "Item/ItemInfoManager.h"
#include "Math/Math.h"
#include "Utils/DialogBuilder.h"

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
