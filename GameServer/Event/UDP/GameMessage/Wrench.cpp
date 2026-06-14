#include "Wrench.h"
#include "Utils/StringUtils.h"
#include "../../../Player/Dialog/WrenchSelfDialog.h"

void Wrench::Execute(GamePlayer* pPlayer, ParsedTextPacket<40>& packet)
{
    if(!pPlayer)
        return;

    auto pNetID = packet.Find("netid"_hash);
    if(!pNetID)
        return;

    uint32 netID = 0;
    if(pNetID->GetUInt(netID) != TO_INT_SUCCESS)
        return;

    if(pPlayer->GetNetID() == netID)
    {
        WrenchSelfDialog::Request(pPlayer);
    }
}