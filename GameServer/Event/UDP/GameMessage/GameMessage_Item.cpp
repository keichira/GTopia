#include "GameMessage_Item.h"
#include "../../../Player/Dialog/PopupDialog.h"
#include "../../../Player/Dialog/TrashDialog.h"
#include "../../../Store/StoreManager.h"
#include "Item/ItemInfoManager.h"
#include "Packet/NetPacket.h"

void GameMessage_Drop(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pItemID = packet.Find("itemID"_hash);
    if (auto pItemID = packet.Find("itemID"_hash))
    {
        uint32 itemID = 0;
        if (pItemID->GetUInt(itemID) != TO_INT_SUCCESS)
            return;

        pPlayer->DropItem(itemID, 1, true);
    }
}

void GameMessage_Trash(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    if (auto pItemID = packet.Find("itemID"_hash))
    {
        uint32 itemID = 0;
        if (pItemID->GetUInt(itemID) != TO_INT_SUCCESS)
            return;

        TrashDialog::Request(pPlayer, itemID);
    }
}

void GameMessage_RefreshItemData(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    pPlayer->SendOnConsoleMessage("One moment updating item data...");

    PlayerLoginDetail& loginDetail = pPlayer->GetLoginDetail();
    ItemsClientData* clientData =
        GetItemInfoManager()->GetClientData(loginDetail.platformType, loginDetail.gameVersion);
    if (!clientData->pItemData)
    {
        pPlayer->SendOnConsoleMessage("Someting went wrong while sending updates");
        pPlayer->LogOff(true, false, true);
        LOGGER_LOG_WARN("Not sending file update data because its NULL");
        return;
    }

    GameUpdatePacket gamePacket;
    gamePacket.type = NET_GAME_PACKET_SEND_ITEM_DATABASE_DATA;
    gamePacket.field_4 = -1;
    gamePacket.field_7 = clientData->size;
    gamePacket.flags |= GAME_PACKET_FLAG_EXTENDED_DATA;
    gamePacket.extraDataSize = clientData->compressSize;

    SendUDPItemDataPacket(pPlayer->GetNetID(), loginDetail.platformType, loginDetail.gameVersion, &gamePacket);
}

void GameMessage_Buy(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pItem = packet.Find("item"_hash);
    if (!pItem)
        return;

    if (pItem->valueSize > 50)
        return;

    StoreManager* pStoreMgr = GetStoreManager();

    StoreEntry* pStoreEntry = pStoreMgr->GetStoreEntryByCode(pItem->GetString());
    if (!pStoreEntry)
        return;

    if (pStoreEntry->isTab)
    {
        eStoreTab storeTab = pStoreMgr->GetTabTypeByCode(pStoreEntry->code);
        pStoreMgr->NavigatePlayer(pPlayer, storeTab);
    }
    else
        pStoreMgr->PurchaseItem(pPlayer, pStoreEntry->code);
}

void GameMessage_Store(GamePlayer* pPlayer, ParsedTextPacket<38>& packet)
{
    if (!pPlayer)
        return;

    auto pLocation = packet.Find("location"_hash);
    if (!pLocation)
        return;

    if (pLocation->valueSize > 50)
        return;

    StoreManager* pStoreMgr = GetStoreManager();
    StoreEntry* pStoreEntry = pStoreMgr->GetStoreEntryByCode(pLocation->GetString());

    if (!pStoreEntry)
        pStoreMgr->NavigatePlayer(pPlayer, STORE_TAB_MAIN_MENU);
    else
    {
        eStoreTab storeTab = pStoreMgr->GetTabTypeByCode(pStoreEntry->code);
        pStoreMgr->NavigatePlayer(pPlayer, storeTab);
    }
}