#include "Buy.h"
#include "../../../Store/StoreManager.h"

void Buy::Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    if(!pPlayer)
        return;

    auto pItem = packet.Find(CompileTimeHashString("item"));
    if(!pItem)
        return;

    if(pItem->size > 50)
        return;

    string incomeItem(pItem->value, pItem->size);
    StoreManager* pStoreMgr = GetStoreManager();

    StoreEntry* pStoreEntry = pStoreMgr->GetStoreEntryByCode(incomeItem);
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