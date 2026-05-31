#include "Trash.h"
#include "../../../Player/Dialog/TrashDialog.h"

void Trash::Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    auto pItemID = packet.Find("itemID"_hash);
    if(!pItemID)
        return;

    uint32 itemID = 0;
    if(ToUInt(string(pItemID->value, pItemID->size), itemID) != TO_INT_SUCCESS)
        return;

    TrashDialog::Request(pPlayer, itemID);
}