#include "Player.h"
#include "../Packet/NetPacket.h"
#include "Proton/ProtonUtils.h"

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

void Player::SendWelcomePacket(uint32 itemsDatHash, const string& cdnServer, const string& cdnPath, const string& settings, uint32 tributeHash)
{
    VariantVector data(7);
    
    string osmHeader;
    if(m_loginDetail.gameVersion <= 2.982) {
        if(m_loginDetail.gameVersion <= 2.479) {
            if(m_loginDetail.gameVersion <= 2.459) {
                if(2.449 < m_loginDetail.gameVersion) {
                    osmHeader = "OnSuperMainStartAcceptLogonFB211131d";
                }
            }
            else {
                osmHeader = "OnSuperMainStartAcceptLogonFB211131dd";
            }
        }
        else {
            osmHeader = "OnSuperMainStartAcceptLogonFB211131ddf";
        }
    }
    else {
        osmHeader = "OnSuperMainStartAcceptLogonHrdxs47254722215a";
    }

    data[0] = osmHeader;
    data[1] = itemsDatHash;
    data[2] = cdnServer;
    data[3] = cdnPath;
    data[4] = "";
    data[5] = settings;
    data[6] = tributeHash;

    SendCallFunctionPacket(data);
}

void Player::SendOnSendToServer(uint16 port, uint32 token, uint32 userID, const string& serverIP)
{
    VariantVector data(6);
    data[0] = "OnSendToServer";
    data[1] = port;
    data[2] = token;
    data[3] = userID;
    data[4] = serverIP + "||";
    data[5] = 1;

    SendCallFunctionPacket(data);
}

void Player::SendOnConsoleMessage(const string &message)
{
    VariantVector data(2);
    data[0] = "OnConsoleMessage";
    data[1] = message;

    SendCallFunctionPacket(data);
}

void Player::SendOnRequestWorldSelectMenu(const string& worldMenu)
{
    VariantVector data(2);
    data[0] = "OnRequestWorldSelectMenu";
    data[1] = worldMenu;

    SendCallFunctionPacket(data);
}

void Player::SendOnFailedToEnterWorld()
{
    VariantVector data(1);
    data[0] = "OnFailedToEnterWorld";

    SendCallFunctionPacket(data);
}

void Player::SendCallFunctionPacket(VariantVector& data, int32 netID, int32 delay)
{
    if(!m_pPeer) {
        return;
    }

    GameUpdatePacket gamePacket;
    gamePacket.type = NET_GAME_PACKET_CALL_FUNCTION;
    gamePacket.flags |= NET_GAME_PACKET_FLAGS_EXTENDED;
    gamePacket.netID = netID;
    gamePacket.delay = delay;

    uint32 size = 0;
    uint8* pData = Proton::SerializeToMem(data, &size, nullptr);
    gamePacket.extraDataSize = size;

    SendENetPacketRaw(NET_MESSAGE_GAME_PACKET, &gamePacket, sizeof(GameUpdatePacket), pData, m_pPeer);
    SAFE_DELETE_ARRAY(pData);
}
