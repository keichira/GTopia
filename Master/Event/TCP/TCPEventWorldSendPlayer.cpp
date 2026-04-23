#include "TCPEventWorldSendPlayer.h"
#include "../../World/WorldManager.h"

void TCPEventWorldSendPlayer::Execute(NetClient* pClient, VariantVector& data)
{
    GetWorldManager()->HandlePlayerJoinRequest(std::move(data));
}
