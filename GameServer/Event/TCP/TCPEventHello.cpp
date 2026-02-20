#include "TCPEventHello.h"
#include "Utils/StringUtils.h"
#include "../../Context.h"

void TCPEventHello::Execute(NetClient* pClient, VariantVector& data)
{
    VariantVector packet(3);
    packet[0] = TCP_PACKET_AUTH;

    /**
     * for now just send back the string
     * actually NetSocket was supporting TLS but removed it for now
     * planned to use HMAC for here for non-TLS socket but openssl lib is so big
     */
    packet[1] = data[1].GetString();
    packet[2] = GetContext()->GetID();

    pClient->Send(packet);
}