#pragma once

#include "../Precompiled.h"
#include "../Network/ENetServer.h"
#include "../Player/Player.h"
#include "../Packet/PacketPool.h"
#include "../Packet/NetPacket.h"

class GamePlayer;

class ServerBase {
public:
    ServerBase();
    virtual ~ServerBase();

public:
    virtual void OnEventConnect(NetworkEvent& event);
    virtual void OnEventReceive(NetworkEvent& event);
    virtual void OnEventDisconnect(NetworkEvent& event);
    virtual void RegisterEvents();
    virtual void Kill();
    virtual void UpdateGameLogic(uint64 maxTimeMS);
    virtual void Update();

public:
    bool Init(const string& host, uint16 port);
    void SetENetIncomeCmdType(uint8 type);

protected:
    ENetServer* m_pENetServer;
    moodycamel::ConcurrentQueue<NetworkEvent> m_networkQueue;

    uint32 m_lastConnectionID;
    std::unordered_map<uint32, ENetPeer*> m_connectionMap;
};