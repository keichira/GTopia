#pragma once

#include "Server/ServerBroadwayBase.h"

class MasterBroadway : public ServerBroadwayBase {
public:
    MasterBroadway();
    ~MasterBroadway();

public:
    static MasterBroadway* GetInstance()
    {
        static MasterBroadway instance;
        return &instance;
    }

public:
    void OnClientConnect(NetClient* pClient) override;

    void Connect(const string& host, uint16 port);
    void RefreshForConnect();

    void SendHelloPacket();

    bool IsConnected() const { return m_connected; }

private:
    NetClient* m_pClient;
    bool m_connected;
};

MasterBroadway* GetMasterBroadway();