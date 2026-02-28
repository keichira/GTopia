#include "RefreshItemData.h"
#include "Packet/NetPacket.h"
#include "Item/ItemInfoManager.h"
#include "IO/Log.h"

void RefreshItemData::Execute(GamePlayer* pPlayer, ParsedTextPacket<8>& packet)
{
    pPlayer->SendOnConsoleMessage("One moment updating item data...");

    ItemsClientData clientData = GetItemInfoManager()->GetClientData(pPlayer->GetLoginDetail().platformType);
    if(!clientData.pItemData) {
        pPlayer->SendOnConsoleMessage("Someting went wrong while sending updates");
        /**
         * disconnect
         */
        LOGGER_LOG_WARN("Not sending file update data because its NULL");
        return;
    }

    GameUpdatePacket gamePacket;
    gamePacket.type = NET_GAME_PACKET_SEND_ITEM_DATABASE_DATA;
    gamePacket.netID = -1;
    gamePacket.flags |= NET_GAME_PACKET_FLAGS_EXTENDED;
    gamePacket.extraDataSize = clientData.size;

    SendENetPacketRaw(NET_MESSAGE_GAME_PACKET, &gamePacket, sizeof(GameUpdatePacket), clientData.pItemData, pPlayer->GetPeer());
}