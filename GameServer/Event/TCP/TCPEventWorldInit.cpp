#include "TCPEventWorldInit.h"
#include "../../World/WorldManager.h"

void TCPEventWorldInit::Execute(NetClient* pClient, VariantVector& data)
{
    GetWorldManager()->OnHandleTCP(std::move(data));
}