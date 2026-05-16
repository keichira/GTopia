#include "Store.h"
#include "../../../Store/StoreManager.h"

void Store::Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    if(!pPlayer)
        return;

    auto pLocation = packet.Find(CompileTimeHashString("location"));
    if(!pLocation)
        return;

    if(pLocation->size > 50)
        return;

    string location(pLocation->value, pLocation->size);

    StoreManager* pStoreMgr = GetStoreManager();
    StoreEntry* pStoreEntry = pStoreMgr->GetStoreEntryByCode(location);

    if(!pStoreEntry)
    {
        pStoreMgr->NavigatePlayer(pPlayer, STORE_TAB_MAIN_MENU);
    }
    else
    {
        eStoreTab storeTab = pStoreMgr->GetTabTypeByCode(pStoreEntry->code);
        pStoreMgr->NavigatePlayer(pPlayer, storeTab);
    }
}