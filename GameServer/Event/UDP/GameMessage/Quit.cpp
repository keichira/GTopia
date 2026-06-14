#include "Quit.h"

void Quit::Execute(GamePlayer* pPlayer, ParsedTextPacket<40>& packet)
{
    if(!pPlayer)
        return;

    pPlayer->LogOff(true, true, true);
}