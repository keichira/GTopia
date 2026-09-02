#pragma once

#include "Event/EventDispatcher.h"
#include "Network/NetEntity.h"
#include "Network/NetSocket.h"
#include "Precompiled.h"
#include "Utils/Timer.h"

template <typename T> class TelnetCommandBase;

struct TelnetClientConfig
{
    string displayName = "";
    string password = "";
    int32 adminLevel = 0;
    std::vector<string> allowedIPs;

    bool IsTrustedIP(const string& ip)
    {
        for (auto& trustedIP : allowedIPs)
        {
            if (trustedIP == ip)
                return true;
        }

        return false;
    }
};

class TelnetClient : public NetEntity
{
public:
    TelnetClient(NetClient* pClient);
    ~TelnetClient();

    void SendMessage(const string& message, bool line);

    void SetAdminLevel(int32 adminLevel) { m_adminLevel = adminLevel; }
    int32 GetAdminLevel() const { return m_adminLevel; }

    void SetDisplayName(const string& displayName) { m_displayName = displayName; }
    string GetDisplayName() const { return m_displayName; }

    Timer& GetLastActionTime() { return m_lastActionTime; }
    string& GetInputBuffer() { return m_inputBuffer; }

    void SetAuthed(bool authed) { m_authed = authed; }
    bool IsAuthed() const { return m_authed; }

    void CloseConnection()
    {
        if (m_pClient)
            m_pClient->status = SOCKET_CLIENT_CLOSE;
    }
    string GetIP() const { return m_pClient ? m_pClient->ip : ""; }

    void IncreasePassTry() { m_passTryCount++; }
    uint8 GetPassTryCount() const { return m_passTryCount; }

    void SetBusy(bool busy) { m_isBusy = busy; }
    bool IsBusy() const { return m_isBusy; }

private:
    NetClient* m_pClient;
    int32 m_adminLevel;
    Timer m_lastActionTime;
    string m_displayName;
    string m_inputBuffer;
    bool m_authed;
    uint8 m_passTryCount;
    bool m_isBusy; // for async
};

class TelnetServer
{
public:
    TelnetServer();
    ~TelnetServer();

public:
    static TelnetServer* GetInstance()
    {
        static TelnetServer instance;
        return &instance;
    }

public:
    bool Init();
    void Kill();

    void Update();

    void OnClientConnect(NetClient* pClient);
    void OnClientReveice(NetClient* pClient);
    void OnClientDisconnect(NetClient* pClient);

    bool LoadTelnetConfigFromFile(const string& filePath);

    TelnetClientConfig* GetClientConfigByPassword(const string& password);
    TelnetClient* GetClientByName(const string& name);
    TelnetClient* GetClientByNetID(uint32 netID);
    void RemoveClient(uint32 netID);

    bool IsTrustedIP(const string& ip);
    bool IsIPRateLimited(const string& ip);
    void ApplyRateLimit(const string& ip);

    void HandleCommand(TelnetClient* pNetClient, const string& command);

    const string& GetHost() const { return m_host; }
    uint16 GetPort() const { return m_port; }

private:
    string m_host;
    uint16 m_port;
    NetSocket* m_pNetSocket;

    bool m_skipIPCheck;
    Timer m_lastClientUpdateTime;

    std::vector<TelnetClientConfig> m_clientConfig;
    std::unordered_map<uint32, TelnetClient*> m_clients;
    std::vector<string> m_trustedIPs;
    std::unordered_map<string, Timer> m_rateLimits;
};

TelnetServer* GetTelnetServer();