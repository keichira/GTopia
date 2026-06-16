#include "Quit.h"

void Quit::Execute(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if(!pPlayer)
        return;

    pPlayer->LogOff(true, true, true);
}