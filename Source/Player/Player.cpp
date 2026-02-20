#include "Player.h"
#include "../Packet/NetPacket.h"

Player::Player(ENetPeer* pPeer)
: m_pPeer(pPeer)
{
    enet_address_get_host_ip(&pPeer->address, m_address, sizeof(m_address));
}

Player::~Player()
{
}

void Player::SendHelloPacket()
{
    if(!m_pPeer) {
        return;
    }

    SendENetPacket(NET_MESSAGE_SERVER_HELLO, "", m_pPeer);
}

void Player::SendLogonFailWithLog(const string& message)
{
    if(!message.empty()) {
        string logAction = "action|log\nmsg|" + message + "\n";
        SendENetPacket(NET_MESSAGE_GAME_MESSAGE, logAction.c_str(), m_pPeer);
    }
    SendENetPacket(NET_MESSAGE_GAME_MESSAGE, "action|logon_fail\n", m_pPeer);
}
