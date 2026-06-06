#include "Player.h"
#include "../Packet/NetPacket.h"
#include "Proton/ProtonUtils.h"
#include "../Utils/Timer.h"

Player::Player()
: m_netID(0), m_userID(0)
{
}

Player::~Player()
{
}

void Player::SendHelloPacket()
{
    SendUDPPacket(GetNetID(), NET_MESSAGE_SERVER_HELLO, "");
}

void Player::SendLogonFailWithLog(const string& message)
{
    if(!message.empty()) 
    {
        string logAction = "action|log\nmsg|" + message + "\n";
        SendUDPPacket(GetNetID(), NET_MESSAGE_GAME_MESSAGE, logAction.c_str());
    }
    SendUDPPacket(GetNetID(), NET_MESSAGE_GAME_MESSAGE, "action|logon_fail\n");
}

void Player::SendWelcomePacket(uint32 itemsDatHash, const string& cdnServer, const string& cdnPath, const string& settings, uint32 tributeHash)
{
    SendCallFunctionPacket(
        GetNetID(),
        VariantPacket::OnWelcomePacket(
            m_loginDetail.protocol, m_loginDetail.gameVersion,
            itemsDatHash, cdnServer, cdnPath, settings, tributeHash
        )
    );
}

void Player::SendOnSendToServer(uint16 port, uint32 token, uint32 userID, const string& serverIP, int32 logonMode)
{
    SendCallFunctionPacket(
        GetNetID(),
        VariantPacket::OnSendToServer(
            port, token, userID, serverIP, logonMode
        )
    );
}

void Player::SendOnConsoleMessage(const string& message)
{
    SendCallFunctionPacket(
        GetNetID(),
        VariantPacket::OnConsoleMessage(message)
    );
}

void Player::SendOnRequestWorldSelectMenu(const string& worldMenu)
{
    SendCallFunctionPacket(
        GetNetID(),
        VariantPacket::OnRequestWorldSelectMenu(worldMenu)
    );
}

void Player::SendOnFailedToEnterWorld()
{
    SendCallFunctionPacket(
        GetNetID(),
        VariantPacket::OnFailedToEnterWorld()
    );
}

void Player::SendOnSpawn(const string& spawnData)
{
    SendCallFunctionPacket(
        GetNetID(),
        VariantPacket::OnSpawn(spawnData)
    );
}

void Player::SendOnChangeSkin(uint32 skinColor, Player* pPlayer)
{
    SendCallFunctionPacket(
        GetNetID(),
        VariantPacket::OnChangeSkin(skinColor),
        pPlayer ? pPlayer->GetNetID() : GetNetID()
    );
}

void Player::SendOnTalkBubble(const string& message, bool stackMessages, Player* pPlayer)
{
    /**
     * check growmojis, player_chat= in here
     */
    uint32 talkerNetID = pPlayer ? (uint32)pPlayer->GetNetID() : (uint32)GetNetID();

    SendCallFunctionPacket(
        GetNetID(),
        VariantPacket::OnTalkBubble(talkerNetID, message, stackMessages)
    );
}

void Player::SendOnSetCurrentWeather(int32 weatherID)
{
    SendCallFunctionPacket(
        GetNetID(),
        VariantPacket::OnSetCurrentWeather(weatherID)
    );
}

void Player::SendOnRemove(int32 netID)
{
    SendCallFunctionPacket(
        GetNetID(),
        VariantPacket::OnRemove(netID)
    );
}

void Player::SendOnDialogRequest(const string& dialogData)
{
    SendCallFunctionPacket(
        GetNetID(),
        VariantPacket::OnDialogRequest(dialogData)
    );
}

void Player::SendOnTextOverlay(const string& message)
{
    SendCallFunctionPacket(
        GetNetID(),
        VariantPacket::OnTextOverlay(message)
    );
}

void Player::SendOnPlayPositioned(const string& fileName, Player* pPlayer)
{
    SendCallFunctionPacket(
        GetNetID(),
        VariantPacket::OnPlayPositioned(fileName),
        pPlayer ? pPlayer->GetNetID() : GetNetID()
    );
}

void Player::SendOnNameChanged(const string& name, Player* pPlayer)
{
    SendCallFunctionPacket(
        GetNetID(),
        VariantPacket::OnNameChanged(name),
        pPlayer ? pPlayer->GetNetID() : GetNetID()
    );
}

void Player::SendSetHasGrowID(bool active, const string& tankIDName, const string& tankIDPass)
{
    SendCallFunctionPacket(
        GetNetID(),
        VariantPacket::SetHasGrowID(active, tankIDName, tankIDPass)
    );
}

void Player::SendSetHasGrowID(bool active)
{
    SendSetHasGrowID(
        active, 
        m_loginDetail.tankIDName.empty() ? m_loginDetail.requestedName : m_loginDetail.tankIDName, 
        m_loginDetail.tankIDPass
    );
}

void Player::SendOnSetBux(uint32 gemCount, bool skipAnim, bool isSupporter, bool isSuperSupporter)
{
    SendCallFunctionPacket(
        GetNetID(),
        VariantPacket::OnSetBux(gemCount, skipAnim, isSupporter, isSuperSupporter, Time::GetSecondsFromMidnight())
    );
}

void Player::SendOnDataConfig(bool isMod, bool isSMod, Player* pPlayer)
{
    SendCallFunctionPacket(
        GetNetID(),
        VariantPacket::OnDataConfig(isMod, isSMod),
        pPlayer ? pPlayer->GetNetID() : GetNetID()
    );
}

void Player::SendOnParticleEffect(eParticleEffect effectType, const Vector2Float& pos, int32 delayMs, float angle)
{
    SendCallFunctionPacket(
        GetNetID(),
        VariantPacket::OnParticleEffect((int32)effectType, pos, angle),
        -1, 
        delayMs
    );
}

void Player::SendOnStoreRequest(const string &storeData)
{
    SendCallFunctionPacket(
        GetNetID(),
        VariantPacket::OnStoreRequest(storeData)
    );
}

void Player::SendOnStorePurchaseResult(const string& resultText)
{
    SendCallFunctionPacket(
        GetNetID(),
        VariantPacket::OnStorePurchaseResult(resultText)
    );
}

void Player::SendOnAction(const string& action, Player* pPlayer)
{
    if(!pPlayer)
    {
        m_lastAction = action;
    }
    
    SendCallFunctionPacket(
        GetNetID(),
        VariantPacket::OnAction(action),
        pPlayer ? pPlayer->GetNetID() : -1
    );
}

void Player::SendOnAddNotification(const string& image, const string& message, const string& audio, bool isTip)
{
    SendCallFunctionPacket(
        GetNetID(),
        VariantPacket::OnAddNotification(image, message, audio, isTip)
    );
}

void Player::SendFakePingReply()
{
    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_PING_REPLY;

    SendUDPPacketRaw(GetNetID(), NET_MESSAGE_GAME_PACKET, &packet, sizeof(GameUpdatePacket), nullptr);
}

void Player::PlaySFX(const string& fileName, int32 delay)
{
    string packet = "action|play_sfx\nfile|audio/" + fileName + "\ndelayMS|" + ToString(delay) + "\n";
    SendUDPPacket(GetNetID(), NET_MESSAGE_GAME_MESSAGE, packet.c_str(), packet.size());
}

#ifdef SERVER_GAME
void Player::SendInventoryPacket()
{
    GameUpdatePacket gamePacket;
    gamePacket.type = NET_GAME_PACKET_SEND_INVENTORY_STATE;
    gamePacket.flags |= GAME_PACKET_FLAG_EXTENDED_DATA;

    uint32 memSize = m_inventory.GetMemEstimate(false);
    gamePacket.extraDataSize = memSize;

    uint8* pData = new uint8[memSize];
    MemoryBuffer memBuffer(pData, memSize);

    m_inventory.Serialize(memBuffer, true, false);

    SendUDPPacketRaw(GetNetID(), NET_MESSAGE_GAME_PACKET, &gamePacket, sizeof(GameUpdatePacket), pData);
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

    data[4] = pPlayer ? (int32)pPlayer->GetCharData().skinColor.GetAsUINTSwap() : (int32)m_characterData.skinColor.GetAsUINTSwap();

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
    SendCallFunctionPacket(GetNetID(), data, netID);
}

void Player::SendCharacterState(Player* pPlayer)
{
    GameUpdatePacket packet;
    packet.type = NET_GAME_PACKET_SET_CHARACTER_STATE;
    packet.field_4 = pPlayer ? pPlayer->GetNetID() : GetNetID();

    CharacterData& charData = pPlayer ? pPlayer->GetCharData() : GetCharData();

    packet.field_1 = charData.punchType;
    packet.field_2 = charData.punchRange;
    packet.field_3 = charData.buildRange;
    packet.flags = charData.char2State;
    packet.field_6 = charData.waterSpeed;
    packet.field_7 = charData.charState;
    packet.field_8.x = charData.speed;
    packet.field_8.y = charData.punchPower;
    packet.field_9.x = charData.accel;
    packet.field_9.y = charData.gravity;

    SendUDPPacketRaw(GetNetID(), NET_MESSAGE_GAME_PACKET, &packet, sizeof(GameUpdatePacket), nullptr);
}

void Player::SendOnSetPos(float x, float y, Player* pPlayer)
{
    SendCallFunctionPacket(
        GetNetID(),
        VariantPacket::OnSetPos(x, y),
        pPlayer ? pPlayer->GetNetID() : -1
    );
}
#endif