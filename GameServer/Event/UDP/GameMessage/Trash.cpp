#include "Trash.h"
#include "../../../Player/Dialog/TrashDialog.h"

void Trash::Execute(GamePlayer* pPlayer, ParsedTextPacket<40>& packet)
{
    auto pItemID = packet.Find("itemID"_hash);
    if(!pItemID)
        return;

    uint32 itemID = 0;
    if(pItemID->GetUInt(itemID) != TO_INT_SUCCESS)
        return;

    TrashDialog::Request(pPlayer, itemID);
}