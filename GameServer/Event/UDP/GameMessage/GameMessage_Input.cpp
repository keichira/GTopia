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
    if (!pText)
        return;

    string text(pText->value, pText->valueSize);

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
