#include "Drop.h"

void Drop::Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    if(!pPlayer)
        return;

    auto pItemID = packet.Find(CompileTimeHashString("itemID"));
    if(!pItemID)
        return;

    uint32 itemID = 0;
    if(ToUInt(string(pItemID->value, pItemID->size), itemID) != TO_INT_SUCCESS)
        return;

    pPlayer->DropItem(itemID, 1, true);
}