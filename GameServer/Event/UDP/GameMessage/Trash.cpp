#include "Trash.h"
#include "../../../Player/Dialog/TrashDialog.h"

void Trash::Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    auto pItemID = packet.Find(CompileTimeHashString("itemID"));
    if(!pItemID)
        return;

    string itemID(pItemID->value, pItemID->size);
    TrashDialog::Request(pPlayer, ToUInt(itemID));
}