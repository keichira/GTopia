#include "Input.h"
#include "../../../World/WorldManager.h"
#include "../../../Server/GameServer.h"
#include "Utils/StringUtils.h"

/**
 * fix here
 */

void Input::Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    if(!pPlayer)
        return;

    World* pWorld = GetWorldManager()->GetWorldByInstanceID(pPlayer->GetCurrentWorld());
    if(!pWorld)
        return;

    auto pText = packet.Find("text"_hash);
    if(!pText)
        return;

    string text(pText->value, pText->size);
    
    RemoveExtraWhiteSpaces(text);
    if(text.empty())
        return;

    if(text[0] == '/') 
    {
        pPlayer->SendOnConsoleMessage("`o" + text);

        auto args = Split(text, ' ');
        GetGameServer()->ExecuteCommand(pPlayer, args);
        return;
    }

    char colorCode = pPlayer->GetRole()->GetChatColor();

    string consoleText = "<" + pPlayer->GetDisplayName(true) + "``> ";
    if(colorCode != 0) 
    {
        consoleText += "`" + string(1, colorCode);
    }
    consoleText += text + "``";

    pWorld->SendTalkBubbleAndConsoleToAll(consoleText, false, pPlayer);
}