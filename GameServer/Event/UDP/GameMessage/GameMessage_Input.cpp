#include "GameMessage_Input.h"
#include "../../../Command/CommandManager.h"
#include "../../../Dialog/DialogManager.h"
#include "../../../Dialog/GameDialogs.h"
#include "../../../Player/GamePlayer.h"
#include "../../../World/WorldManager.h"

void GameMessage_DialogReturn(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    GetDialogManager()->Execute(pPlayer, packet);
}

void GameMessage_SetSkin(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    if (auto pColor = packet.Find("color"_hash))
    {
        uint32 skinColor = 0;
        if (pColor->GetUInt(skinColor) != TO_INT_SUCCESS)
            return;

        pPlayer->GetCharData().SetSkinColor(pPlayer->NormalizeSkinColor(skinColor));
    }
}

void GameMessage_Input(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if (!pWorld)
        return;

    auto pText = packet.Find("text"_hash);
    if (!pText || pText->valueSize == 0)
        return;

    int32 textLen = Min(120, pText->valueSize);
    string text(pText->value, textLen);

    RemoveExtraWhiteSpaces(text);
    if (text.empty())
        return;

    if (text[0] == '/')
    {
        pPlayer->SendOnConsoleMessage("`o" + text);

        auto args = Split(text, ' ');
        GetCommandManager()->ExecuteCommand(pPlayer, args);
        return;
    }

    GTRemoveDangerousFromChat(text);

    string plainText = text;
    RemoveGTColorCodes(plainText);

    if (plainText.empty() || plainText == " ")
        return;

    TileInfo* pWorldLockTile = pWorld->GetTileManager()->GetKeyTile(KEY_TILE_WORLD_LOCK);
    if (pWorldLockTile && pWorldLockTile->GetFG() == ITEM_ID_ROYAL_LOCK)
    {
        TileExtra_Lock* pTileExtra = pWorldLockTile->GetExtra<TileExtra_Lock>();
        if (pTileExtra && pTileExtra->HasFlag(TILE_EXTRA_LOCK_SILENCE) && !pTileExtra->HasAccess(pPlayer->GetUserID()))
        {
            pPlayer->SendOnTalkBubble("`1(Peasants must not speak)``", false);
            return;
        }
    }

    if (pPlayer->GetModController().HasPlayMod(PLAYMOD_TYPE_DUCT_TAPE) ||
        pPlayer->GetInventory().GetClothByPart(BODY_PART_FACEITEM) == ITEM_ID_IRON_MASK)
    {
        for (auto& c : plainText)
        {
            if (c >= 'a' && c <= 'z')
            {
                c = (RandomRangeInt(0, 1) == 0) ? 'f' : 'm';
            }
            else if (c >= 'A' && c <= 'Z')
            {
                c = (RandomRangeInt(0, 1) == 0) ? 'F' : 'M';
            }
            else if ((c < '!' || c > '\'') && c != '?')
            {
                c = ' ';
            }
        }
    }

    Role* pRole = pPlayer->GetRole();
    string message = text;
    if (pRole && pRole->GetChatColor() != '\0')
        message = "`" + string(1, pRole->GetChatColor()) + text;

    pWorld->SendTalkBubbleAndConsoleToAll(message, false, pPlayer);
}

void GameMessage_Wrench(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    if (auto pNetID = packet.Find("netid"_hash))
    {
        uint32 netID = 0;
        if (pNetID->GetUInt(netID) != TO_INT_SUCCESS)
            return;

        if (pPlayer->GetNetID() == netID)
            PopupDialog::RequestSelf(pPlayer);
    }
}
