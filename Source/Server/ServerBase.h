#pragma once

#include "../Precompiled.h"
#include "../Network/ENetServer.h"
#include "../Player/Player.h"

class ServerBase {
public:
    typedef std::unordered_map<int32, Player*> PlayerCache;

public:
    ServerBase();
    virtual ~ServerBase();

public:
    virtual void OnEventConnect(ENetEvent& event);
    virtual void OnEventReceive(ENetEvent& event);
    virtual void OnEventDisconnect(ENetEvent& event);
    virtual void RegisterEvents();
    virtual void Kill();

public:
    bool Init(const string& host, uint16 port);
    void Update();
    void UpdateGameLogic(uint64 maxTimeMS);

    Player* GetPlayerByNetID(int32 netID);
    //PlayerCache& GetPlayerCache() { return m_playerCache; }

protected:
    ENetServer* m_pENetServer;
    PlayerCache m_playerCache;
};