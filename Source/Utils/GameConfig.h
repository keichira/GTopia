#pragma once

#include "../Precompiled.h"

enum eConfigServerType
{
    CONFIG_SERVER_MASTER,
    CONFIG_SERVER_GAME,
    CONFIG_SERVER_RENDERER,
};

struct ServerConfigSchema
{
    uint16 id;
    string lanIP;
    string wanIP;
    uint16 tcpPort;
    uint16 udpPort;
    eConfigServerType serverType;
};

struct DatabaseConfigSchema
{
    string host;
    string user;
    string pass;
    string database;
    uint16 port;
};

struct WorldBalanceConfigSchema
{
    string worldName;
    string fileName;
    int32 maxInstance = 2;
    bool keepExactId = false;
};

class GameConfig {
public:
    GameConfig();

public:
    bool LoadConfig(const string& filePath);

    uint16 LoadServersMaster(const string& filePath);
    uint16 LoadServersClient(const string& filePath, uint16 serverID);

private:
    void AddServer(uint16 serverID, const string& lanIP, const string& wanIP, uint16 tcpStart, uint16 udpStart, eConfigServerType serverType);

public:
    std::vector<ServerConfigSchema> servers;
    DatabaseConfigSchema database;

    string cdnServer = "";
    string cdnPath = "";
    string worldSavePath = "";
    string rendererSavePath = "";
    string rendererStaticPath = "";
    uint16 maxLoginsAtOnce = 20;
    uint16 maxAccountsPerIP = 3;
    uint16 maxAccountsPerGid = 3;
    uint16 maxAccountsPerVid = 3;
    uint16 maxAccountsPerSid = 3;
    uint16 maxAccountsPerMac = 3;
    uint32 worldMaxPlayerCount = 50;
    uint8 enetIncomeCmdType = 3;

    uint16 forceItemDataVersion = 0;
    float androidSupportedVersions[2];
    float windowsSupportedVersions[2];
    float iosSupportedVersions[2];
    float macosSupportedVersions[2];

    bool enableTelnetServer = false;

    bool isWorldBalancerEnabled = false;
    std::vector<WorldBalanceConfigSchema> balancedWorlds;
    float balanceSoftCapRatio = 0.6;
};