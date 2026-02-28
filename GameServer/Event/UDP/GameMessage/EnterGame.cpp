#include "EnterGame.h"

void EnterGame::Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    pPlayer->SendOnRequestWorldSelectMenu("");
}