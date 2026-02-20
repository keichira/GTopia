#include "TCPEventHello.h"
#include "../../Server/ServerManager.h"
#include "Utils/StringUtils.h"

void TCPEventHello::Execute(NetClient* pClient, VariantVector& data)
{
    if(data.size() != 1) {
        pClient->status = SOCKET_CLIENT_CLOSE;
        return;
    }

    pClient->data = new NetClientInfo();

    uint8 bytes[16];
    if(GetRandomBytes(&bytes, sizeof(bytes)) < 0) {
        pClient->status = SOCKET_CLIENT_CLOSE;
        return;
    }

    VariantVector packet(2);
    packet[0] = TCP_PACKET_HELLO;
    packet[1] = ToHex(bytes, sizeof(bytes));

    ((NetClientInfo*)pClient->data)->authKey = packet[1].GetString();
    pClient->Send(packet);
}