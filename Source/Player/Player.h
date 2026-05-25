#pragma once

#include "../Precompiled.h"
#include "../Network/NetEntity.h"
#include "PlayerLoginDetail.h"
#include "PlayerInventory.h"
#include "CharacterData.h"
#include "../Utils/Variant.h"
#include "ParticleEffect.h"
#include <enet/enet.h>


enum ePlayerLogonMode
{
    LOGON_MODE_WELCOME = 1,
    LOGON_MODE_TRANSFER = 2
};

class Player : public NetEntity {
public:
    Player(ENetPeer* pPeer);
    virtual ~Player();

public:
    void SendHelloPacket();
    void SendLogonFailWithLog(const string& message);
    void SendWelcomePacket(uint32 itemsDatHash, const string& cdnServer, const string& cdnPath, const string& settings, uint32 tributeHash);
    void SendOnSendToServer(uint16 port, uint32 token, uint32 userID, const string& serverIP, int32 logonMode);
    void SendOnConsoleMessage(const string& message);
    void SendOnRequestWorldSelectMenu(const string& worldMenu);
    void SendOnFailedToEnterWorld();
    void SendOnSpawn(const string& spawnData);
    void SendOnChangeSkin(uint32 skinColor, Player* pPlayer = nullptr);
    void SendOnTalkBubble(const string& message, bool stackMessages, Player* pPlayer = nullptr);
    void SendOnSetCurrentWeather(int32 weatherID);
    void SendOnRemove(int32 netID);
    void SendOnDialogRequest(const string& dialogData);
    void SendOnTextOverlay(const string& message);
    void SendOnPlayPositioned(const string& fileName, Player* pPlayer = nullptr);
    void SendOnNameChanged(const string& name, Player* pPlayer);
    void SendSetHasGrowID(bool active, const string& tankIDName, const string& tankIDPass);
    void SendSetHasGrowID(bool active);
    void SendOnSetBux(uint32 gemCount, bool skipAnim, bool isSupporter, bool isSuperSupporter);
    void SendOnDataConfig(bool isMod, bool isSMod, Player* pPlayer = nullptr);
    void SendOnParticleEffect(eParticleEffect effectType, const Vector2Float& pos, int32 delayMS = 0, float angle = 0.0f);
    void SendOnStoreRequest(const string& storeData);
    void SendOnStorePurchaseResult(const string& resultText);
    void SendFakePingReply();

    void PlaySFX(const string& fileName, int32 delay = -1);
    void SendCallFunctionPacket(VariantVector& data, int32 netID = -1, int32 delay = -1);

    PlayerLoginDetail& GetLoginDetail() { return m_loginDetail; }
    ENetPeer* GetPeer() { return m_pPeer; }

    uint32 GetUserID() const { return m_userID; }
    void SetUserID(uint32 userID) { m_userID = userID; }

    string GetAddress() const { return m_address; }

#ifdef SERVER_GAME
    PlayerInventory& GetInventory() { return m_inventory; }
    void SendInventoryPacket();

    CharacterData& GetCharData() { return m_characterData; }
    void SendOnSetClothing(Player* pPlayer = nullptr);
    void SendCharacterState(Player* pPlayer = nullptr);
    void SendOnSetPos(float x, float y, Player* pPlayer = nullptr);
#endif

protected:
    uint32 m_userID;
    PlayerLoginDetail m_loginDetail;

    ENetPeer* m_pPeer;
    char m_address[16];

#ifdef SERVER_GAME
    PlayerInventory m_inventory;
    CharacterData m_characterData;
#endif
};