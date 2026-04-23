#pragma once

#include "../Precompiled.h"
#include "../Network/ENetServer.h"
#include "../Player/Player.h"

class GamePlayer;

class ServerBase {
public:
    ServerBase();
    virtual ~ServerBase();

public:
    virtual void OnEventConnect(ENetEvent& event);
    virtual void OnEventReceive(ENetEvent& event);
    virtual void OnEventDisconnect(ENetEvent& event);
    virtual void RegisterEvents();
    virtual void Kill();
    virtual void UpdateGameLogic(uint64 maxTimeMS);

public:
    bool Init(const string& host, uint16 port);
    void Update();

protected:
    ENetServer* m_pENetServer;
};