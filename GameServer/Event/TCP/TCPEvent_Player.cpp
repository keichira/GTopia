#include "TCPEvent_Player.h"
#include "../../Player/GamePlayer.h"
#include "../../Player/PlayerManager.h"

void TCPEvent_PlayerCheckSession(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    int32 playerNetID = 0;
    if (!reader.Read<int32>(playerNetID) || playerNetID <= 0)
        return;

    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(playerNetID);
    if (!pPlayer)
        return;

    pPlayer->HandleCheckSession(reader);
}