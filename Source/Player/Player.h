#pragma once

#include "../Precompiled.h"
#include "../Network/NetEntity.h"
#include "PlayerLoginDetail.h"
#include <enet/enet.h>

class Player : public NetEntity {
public:
    Player(ENetPeer* pPeer);
    virtual ~Player();

public:
    void SendHelloPacket();
    void SendLogonFailWithLog(const string& message);

    ENetPeer* GetPeer() { return m_pPeer; }

protected:
    uint32 m_userID;
    PlayerLoginDetail m_loginDetail;

    ENetPeer* m_pPeer;
    char m_address[16];
};