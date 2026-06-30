#include "TCPEvent_Player.h"
#include "../../Player/GamePlayer.h"
#include "../../Player/PlayerManager.h"

void TCPEvent_PlayerCheckSession(NetClient* pClient, VariantVector& data)
{
    if (!pClient || data.size() < 2)
        return;

    int32 playerNetID = data[1].GetINT();

    if (playerNetID <= 0)
        return;

    GamePlayer* pPlayer = GetPlayerManager()->GetPlayerByNetID(playerNetID);
    if (!pPlayer)
        return;

    pPlayer->HandleCheckSession(std::move(data));
}