#pragma once

#include "../Precompiled.h"

struct ServerConfigSchema
{
    uint16 id;
    string lanIP;
    string wanIP;
    uint16 tcpPort;
    uint16 udpPort;
};

struct DatabaseConfigSchema
{
    string host;
    string user;
    string pass;
    string database;
    uint16 port;
};

class GameConfig {
public:
    GameConfig();

public:
    bool LoadConfig(const string& filePath);

#ifdef SERVER_MASTER
    uint16 LoadServers(const string& filePath);
#else
    uint16 LoadServers(const string& filePath, uint16 serverID);
#endif

public:
    std::vector<ServerConfigSchema> servers;
    DatabaseConfigSchema database;

    string cdnServer = "";
    string cdnPath = "";
    string worldSavePath = "";
};