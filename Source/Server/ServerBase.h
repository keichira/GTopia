#pragma once

#include "../Network/ENetServer.h"
#include "../Packet/NetPacket.h"
#include "../Packet/PacketPool.h"
#include "../Player/Player.h"
#include "../Precompiled.h"

struct PeerConnection
{
    ENetPeer* pPeer;
    uint64 startMS;
    uint32 packetCount;
};

class GamePlayer;

class ServerBase
{
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

    void WaitForNetwork();
    void KickPeer(uint32 netID, ENetPeer* pPeer);
    bool BudgetExpired(uint32 processed, uint64 startUS);

protected:
    ENetServer* m_pENetServer;
    moodycamel::ConcurrentQueue<NetworkEvent> m_networkQueue;

    uint32 m_lastConnectionID;
    std::unordered_map<uint32, PeerConnection> m_connectionMap;
};