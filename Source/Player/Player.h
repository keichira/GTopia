#pragma once

#include "../Precompiled.h"
#include "../Network/NetEntity.h"
#include "PlayerLoginDetail.h"
#include "PlayerInventory.h"
#include <enet/enet.h>

class Player : public NetEntity {
public:
    Player(ENetPeer* pPeer);
    virtual ~Player();

public:
    void SendHelloPacket();
    void SendLogonFailWithLog(const string& message);
    void SendWelcomePacket(uint32 itemsDatHash, const string& cdnServer, const string& cdnPath, const string& settings, uint32 tributeHash);
    void SendOnSendToServer(uint16 port, uint32 token, uint32 userID, const string& serverIP);
    void SendOnConsoleMessage(const string& message);
    void SendOnRequestWorldSelectMenu(const string& worldMenu);
    void SendOnFailedToEnterWorld();

    void SendCallFunctionPacket(VariantVector& data, int32 netID = -1, int32 delay = -1);

    const PlayerLoginDetail& GetLoginDetail() const { return m_loginDetail; }
    ENetPeer* GetPeer() { return m_pPeer; }

#ifdef SERVER_GAME
    const PlayerInventory& GetInventory() const { return m_inventory; }
#endif

protected:
    uint32 m_userID;
    PlayerLoginDetail m_loginDetail;

    ENetPeer* m_pPeer;
    char m_address[16];

#ifdef SERVER_GAME
    PlayerInventory m_inventory;
#endif
};