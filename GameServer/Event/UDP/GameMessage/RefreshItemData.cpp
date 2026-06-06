#include "RefreshItemData.h"
#include "Packet/NetPacket.h"
#include "Item/ItemInfoManager.h"
#include "IO/Log.h"

void RefreshItemData::Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    pPlayer->SendOnConsoleMessage("One moment updating item data...");

    ItemsClientData* clientData = GetItemInfoManager()->GetClientData(pPlayer->GetLoginDetail().platformType, pPlayer->GetLoginDetail().gameVersion);
    if(!clientData->pItemData) 
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

    SendUDPPacketRaw(pPlayer->GetNetID(), NET_MESSAGE_GAME_PACKET, &gamePacket, sizeof(GameUpdatePacket), clientData->pItemData);
}