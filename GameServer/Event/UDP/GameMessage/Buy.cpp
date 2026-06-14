#include "Buy.h"
#include "../../../Store/StoreManager.h"

void Buy::Execute(GamePlayer* pPlayer, ParsedTextPacket<40>& packet)
{
    if(!pPlayer)
        return;

    auto pItem = packet.Find("item"_hash);
    if(!pItem)
        return;

    if(pItem->size > 50)
        return;

    StoreManager* pStoreMgr = GetStoreManager();

    StoreEntry* pStoreEntry = pStoreMgr->GetStoreEntryByCode(pItem->GetString());
    if(!pStoreEntry)
        return;

    if(pStoreEntry->isTab)
    {
        eStoreTab storeTab = pStoreMgr->GetTabTypeByCode(pStoreEntry->code);
        pStoreMgr->NavigatePlayer(pPlayer, storeTab);
    }
    else
    {
        pStoreMgr->PurchaseItem(pPlayer, pStoreEntry->code);
    }
}