#include "Drop.h"

void Drop::Execute(GamePlayer* pPlayer, ParsedTextPacket<40>& packet)
{
    if(!pPlayer)
        return;

    auto pItemID = packet.Find("itemID"_hash);
    if(!pItemID)
        return;

    uint32 itemID = 0;
    if(pItemID->GetUInt(itemID) != TO_INT_SUCCESS)
        return;

    pPlayer->DropItem(itemID, 1, true);
}