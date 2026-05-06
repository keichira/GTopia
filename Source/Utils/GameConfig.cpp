#include "GameConfig.h"
#include "../IO/File.h"
#include "StringUtils.h"

#include "../IO/Log.h"

GameConfig::GameConfig()
{
}

bool GameConfig::LoadConfig(const string& filePath)
{
    File file;
    if(!file.Open(filePath)) {
        return false;
    }

    uint32 fileSize = file.GetSize();
    string fileData(fileSize, '\0');

    if(file.Read(fileData.data(), fileSize) != fileSize) {
        return false;
    }

    auto lines = Split(fileData, '\n');

    for(auto& line : lines) {
        if(line.empty() || line[0] == '#') {
            continue;
        }

        auto args = Split(line, '|');

        if(args[0] == "database_info") {
            database.host = args[1];
            database.user = args[2];
            database.pass = args[3];
            database.database = args[4];
            database.port = (uint16)ToUInt(args[5]);
        }

        if(args[0] == "cdn_server") {
            cdnServer = args[1];
            cdnPath = args[2];
        }

        if(args[0] == "rendererStaticPath") {
            rendererStaticPath = args[1];
        }

        if(args[0] == "rendererSavePath") {
            rendererSavePath = args[1];
        }

        if(args[0] == "worldSavePath") {
            worldSavePath = args[1];
        }

        if(args[0] == "max_logins_at_once") {
            maxLoginsAtOnce = (uint16)ToUInt(args[1]);
        }

        if(args[0] == "max_accounts_per_ip") {
            maxAccountsPerIP = (uint16)ToUInt(args[1]);
        }

        if(args[0] == "max_accounts_per_gid") {
            maxAccountsPerGid = (uint16)ToUInt(args[1]);
        }

        if(args[0] == "max_accounts_per_vid") {
            maxAccountsPerVid = (uint16)ToUInt(args[1]);
        }

        if(args[0] == "max_accounts_per_sid") {
            maxAccountsPerSid = (uint16)ToUInt(args[1]);
        }

        if(args[0] == "max_accounts_per_mac") {
            maxAccountsPerMac = (uint16)ToUInt(args[1]);
        }

        if(args[0] == "world_max_player_count") {
            worldMaxPlayerCount = ToUInt(args[1]);
        }

        if(args[0] == "enet_income_cmd_type") {
            enetIncomeCmdType = (uint8)ToUInt(args[1]);
        }

        if(args[0] == "force_item_data_version") {
            forceItemDataVersion = (uint16)ToUInt(args[1]);
        }

        if(args[0] == "android_supported_versions") {
            androidSupportedVersions[0] = ToFloat(args[1]);
            androidSupportedVersions[1] = ToFloat(args[2]);
        }

        if(args[0] == "windows_supported_versions") {
            windowsSupportedVersions[0] = ToFloat(args[1]);
            windowsSupportedVersions[1] = ToFloat(args[2]);
        }

        if(args[0] == "ios_supported_versions") {
            iosSupportedVersions[0] = ToFloat(args[1]);
            iosSupportedVersions[1] = ToFloat(args[2]);
        }

        if(args[0] == "macos_supported_versions") {
            macosSupportedVersions[0] = ToFloat(args[1]);
            macosSupportedVersions[1] = ToFloat(args[2]);
        }
    }

    return true;
}

uint16 GameConfig::LoadServersMaster(const string &filePath)
{
    File file;
    if(!file.Open(filePath)) {
        return 0;
    }

    uint32 fileSize = file.GetSize();
    string fileData(fileSize, '\0');
    if(file.Read(fileData.data(), fileSize) != fileSize) {
        return 0;
    }

    auto lines = Split(fileData, '\n');

    uint16 tcpStart = 18500;
    uint16 udpStart = 18000;

    for(auto& line : lines) {
        if(line.empty() || line[0] == '#') {
            continue;
        }

        auto args = Split(line, '|');

        if(args[0] == "tcp_start") {
            tcpStart = (uint16)ToUInt(args[1]);
        }

        else if(args[0] == "udp_start") {
            udpStart = (uint16)ToUInt(args[1]);
        }

        else if(args[0] == "set_master") {
            AddServer(0, args[1], args[2], tcpStart, udpStart, CONFIG_SERVER_MASTER);
        }

        else if(args[0] == "add_server" || args[0] == "add_renderer") {
            uint16 serverCount = (uint16)ToUInt(args[3]);
            eConfigServerType type = CONFIG_SERVER_GAME;

            if(args[0] == "add_renderer") {
                type = CONFIG_SERVER_RENDERER;
            }

            for(uint16 i = 0; i < serverCount; ++i) {
                uint16 serverID = servers.size();
                AddServer(serverID, args[1], args[2], tcpStart, udpStart, type);
            }
        }
    }

    return servers.size();
}

uint16 GameConfig::LoadServersClient(const string& filePath, uint16 serverID)
{
    File file;
    if(!file.Open(filePath)) {
        return 0;
    }

    uint32 fileSize = file.GetSize();
    string fileData(fileSize, '\0');
    if(file.Read(fileData.data(), fileSize) != fileSize) {
        return 0;
    }

    auto lines = Split(fileData, '\n');

    uint16 tcpStart = 18500;
    uint16 udpStart = 18000;
    uint32 currServerIdEnd = 0;

    for(auto& line : lines) {
        if(line.empty() || line[0] == '#') {
            continue;
        }

        auto args = Split(line, '|');

        if(args[0] == "tcp_start") {
            tcpStart = (uint16)ToUInt(args[1]);
        }

        else if(args[0] == "udp_start") {
            udpStart = (uint16)ToUInt(args[1]);
        }

        else if(args[0] == "set_master") {
            AddServer(0, args[1], args[2], tcpStart, udpStart, CONFIG_SERVER_MASTER);
        }

        else if(args[0] == "add_server" || args[0] == "add_renderer") {
           uint16 serverCount = (uint16)ToUInt(args[3]);
           uint32 start = currServerIdEnd + 1;
           currServerIdEnd = start + serverCount - 1;
       
           if(serverID >= start && serverID <= currServerIdEnd) {
               eConfigServerType type = CONFIG_SERVER_GAME;

                if(args[0] == "add_renderer") {
                    type = CONFIG_SERVER_RENDERER;
                }
                AddServer(serverID, args[1], args[2], tcpStart, udpStart, type);
                break;
           }
       }
    }

    return servers.size();
}

void GameConfig::AddServer(uint16 serverID, const string& lanIP, const string& wanIP, uint16 tcpStart, uint16 udpStart, eConfigServerType serverType)
{
    servers.emplace_back(ServerConfigSchema{
        serverID, lanIP, wanIP,
        (uint16)(tcpStart + serverID),
        (uint16)(udpStart + serverID),
        serverType
    });
}
