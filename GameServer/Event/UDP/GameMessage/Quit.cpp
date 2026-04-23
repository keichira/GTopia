#include "Quit.h"

void Quit::Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    if(!pPlayer) {
        return;
    }

    pPlayer->LogOff(true);
}