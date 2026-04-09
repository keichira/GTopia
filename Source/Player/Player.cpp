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
    VariantVector data;
    if(m_loginDetail.protocol < 93) {
        data = VariantVector(6);
    }
    else {
        data = VariantVector(7);
    }
    
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
    
    if(m_loginDetail.protocol > 93) {
        data[6] = tributeHash;
    }

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

void Player::SendOnConsoleMessage(const string& message)
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

void Player::SendOnSpawn(const string& spawnData)
{
    VariantVector data(2);
    data[0] = "OnSpawn";
    data[1] = spawnData;

    SendCallFunctionPacket(data);
}

void Player::SendOnChangeSkin(uint32 skinColor, Player* pPlayer)
{
    VariantVector data(2);
    data[0] = "OnChangeSkin";
    data[1] = skinColor;

    SendCallFunctionPacket(data, pPlayer ? pPlayer->GetNetID() : GetNetID());
}

void Player::SendOnTalkBubble(const string& message, bool stackMessages, Player* pPlayer)
{

    /**
     * check growmojis, player_chat= in here
     */

    VariantVector data(5);
    data[0] = "OnTalkBubble";
    data[1] = pPlayer ? (uint32)pPlayer->GetNetID() : (uint32)GetNetID();
    data[2] = message;
    data[3] = (uint32)0; // 2 for green box
    data[4] = stackMessages ? (uint32)1 : (uint32)0;

    SendCallFunctionPacket(data);
}

void Player::SendOnSetCurrentWeather(int32 weatherID)
{
    VariantVector data(2);
    data[0] = "OnSetCurrentWeather";
    data[1] = weatherID;

    SendCallFunctionPacket(data);
}

void Player::SendOnRemove(int32 netID)
{
    VariantVector data(2);
    data[0] = "OnRemove";
    data[1] = "netID|" + ToString(netID) + "\n";

    SendCallFunctionPacket(data);
}

void Player::SendOnDialogRequest(const string& dialogData)
{
    VariantVector data(2);
    data[0] = "OnDialogRequest";
    data[1] = dialogData;

    SendCallFunctionPacket(data);
}

void Player::SendOnTextOverlay(const string& message)
{
    VariantVector data(2);
    data[0] = "OnTextOverlay";
    data[1] = message;

    SendCallFunctionPacket(data);
}

void Player::SendOnPlayPositioned(const string& fileName, Player* pPlayer)
{
    VariantVector data(2);
    data[0] = "OnPlayPositioned";
    data[1] = "audio/" + fileName;

    SendCallFunctionPacket(data, pPlayer ? pPlayer->GetNetID() : GetNetID());
}

void Player::SendOnNameChanged(const string& name, Player* pPlayer)
{
    VariantVector data(2);
    data[0] = "OnNameChanged";
    data[1] = name;

    SendCallFunctionPacket(data, pPlayer ? pPlayer->GetNetID() : GetNetID());
}

void Player::SendFakePingReply()
{
    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_PING_REPLY;

    SendENetPacketRaw(NET_MESSAGE_GAME_PACKET, &packet, sizeof(GameUpdatePacket), nullptr, m_pPeer);
}

void Player::PlaySFX(const string& fileName, int32 delay)
{
    string packet = "action|play_sfx\nfile|audio/" + fileName + "\ndelayMS|" + ToString(delay) + "\n";
    SendENetPacket(NET_MESSAGE_GAME_MESSAGE, packet.c_str(), m_pPeer);
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

#ifdef SERVER_GAME
void Player::SendInventoryPacket()
{
    if(!m_pPeer) {
        return;
    }

    GameUpdatePacket gamePacket;
    gamePacket.type = NET_GAME_PACKET_SEND_INVENTORY_STATE;
    gamePacket.flags |= NET_GAME_PACKET_FLAGS_EXTENDED;

    uint32 memSize = m_inventory.GetMemEstimate(false);
    gamePacket.extraDataSize = memSize;

    uint8* pData = new uint8[memSize];
    MemoryBuffer memBuffer(pData, memSize);

    m_inventory.Serialize(memBuffer, true, false);

    SendENetPacketRaw(NET_MESSAGE_GAME_PACKET, &gamePacket, sizeof(GameUpdatePacket), pData, m_pPeer);
    SAFE_DELETE_ARRAY(pData);
}

void Player::SendOnSetClothing(Player* pPlayer)
{
    auto clothes = pPlayer ? pPlayer->GetInventory().GetClothes() : m_inventory.GetClothes();

    VariantVector data(6);
    data[0] = "OnSetClothing";
    data[1] = Vector3Float(clothes[BODY_PART_HAIR], clothes[BODY_PART_SHIRT], clothes[BODY_PART_PANT] );
    data[2] = Vector3Float(clothes[BODY_PART_SHOE], clothes[BODY_PART_FACEITEM], clothes[BODY_PART_HAND]);
    data[3] = Vector3Float(clothes[BODY_PART_BACK], clothes[BODY_PART_HAT], clothes[BODY_PART_CHESTITEM]);

    data[4] = pPlayer ? (int32)pPlayer->GetCharData().GetSkinColor(true) : (int32)m_characterData.GetSkinColor(true);

    bool isInvis = true;

    if(m_loginDetail.gameVersion < 2.62) {
        data[5] = uint32(isInvis ? 1 : 0);
    }
    else if(m_loginDetail.protocol < 32) {
        data[5] = Vector3Float(isInvis ? 1.0 : 0.0, 0, 0);
    }
    else {
        float artifact = 0;
        data[5] = Vector3Float(artifact, isInvis ? 1.0 : 0.0, 0);
    }

    int32 netID = pPlayer ? pPlayer->GetNetID() : GetNetID();
    SendCallFunctionPacket(data, netID);
}

void Player::SendCharacterState(Player* pPlayer)
{
    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_SET_CHARACTER_STATE;
    packet.netID = pPlayer ? pPlayer->GetNetID() : GetNetID();

    CharacterData& charData = pPlayer ? pPlayer->GetCharData() : GetCharData();

    packet.characterPunchType = charData.GetPunchType();
    packet.buildRange = charData.GetBuildRange();
    packet.punchRange = charData.GetPunchRange();
    packet.characterFlags = (int32)charData.GetCharFlags();

    packet.characterWaterSpeed = charData.GetWaterSpeed();
    packet.characterSpeed = charData.GetSpeed();
    packet.characterPunchPower = charData.GetPunchPower();
    packet.characterAccel = charData.GetAccel();
    packet.characterGravity = charData.GetGravity();
    packet.characterFireDamage = charData.GetFireDamage();

    packet.Print();

    SendENetPacketRaw(NET_MESSAGE_GAME_PACKET, &packet, sizeof(GameUpdatePacket), nullptr, m_pPeer);
}

void Player::SendOnSetPos(float x, float y, Player* pPlayer)
{
    VariantVector data(2);
    data[0] = "OnSetPos";
    data[1] = Vector2Float(x, y);

    SendCallFunctionPacket(data, pPlayer ? pPlayer->GetNetID() : GetNetID());
}
#endif