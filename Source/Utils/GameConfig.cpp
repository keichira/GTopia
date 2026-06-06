#include "GameConfig.h"
#include "../IO/File.h"
#include "StringUtils.h"
#include "../IO/Log.h"
#include "ConfigDB.h"

GameConfig::GameConfig()
{
}

bool GameConfig::LoadConfig(const string& filePath)
{
    ConfigDB cfg;
    if(!cfg.Load(filePath))
        return false;

    for(const auto& line : cfg.Lines())
    {
        const string& key = line.GetString(0);
    
        if(key == "database_info")
        {
            if(!line.Require(5))
                return false;

            database.host = line.GetString(1);
            database.user = line.GetString(2);
            database.pass = line.GetString(3);
            database.database = line.GetString(4);
            database.port = line.GetUInt(5, 3306);
    
            continue;
        }
    
        if(key == "cdn_server")
        {
            if(!line.Require(2))
                return false;

            cdnServer = line.GetString(1);
            cdnPath = line.GetString(2);
    
            continue;
        }
    
        if(key == "renderer_static_path")
        {
            rendererStaticPath = line.GetString(1);
    
            continue;
        }
    
        if(key == "renderer_save_path")
        {
            rendererSavePath = line.GetString(1);
    
            continue;
        }
    
        if(key == "world_save_path")
        {
            if(line.Require(1))
                worldSavePath = line.GetString(1);
    
            continue;
        }
    
        if(key == "max_logins_at_once")
        {
            maxLoginsAtOnce = line.GetUInt(1, 20);
            continue;
        }
    
        if(key == "max_accounts_per_ip")
        {
            maxAccountsPerIP = line.GetUInt(1, 5);
            continue;
        }
    
        if(key == "max_accounts_per_gid")
        {
            maxAccountsPerGid = line.GetUInt(1, 5);
            continue;
        }
    
        if(key == "max_accounts_per_vid")
        {
            maxAccountsPerVid = line.GetUInt(1, 5);
            continue;
        }
    
        if(key == "max_accounts_per_sid")
        {
            maxAccountsPerSid = line.GetUInt(1, 5);
            continue;
        }
    
        if(key == "max_accounts_per_mac")
        {
            maxAccountsPerMac = line.GetUInt(1, 5);
            continue;
        }
    
        if(key == "world_max_player_count")
        {
            worldMaxPlayerCount = line.GetUInt(1, 70);
            continue;
        }
    
        if(key == "enet_income_cmd_type")
        {
            enetIncomeCmdType = line.GetUInt(1, 3);
            continue;
        }
    
        if(key == "force_item_data_version")
        {
            forceItemDataVersion = line.GetUInt(1, 5);
            continue;
        }
    
        if(key == "android_supported_versions")
        {
            if(line.Require(2))
            {
                androidSupportedVersions[0] = line.GetFloat(1, 3.02f);
                androidSupportedVersions[1] = line.GetFloat(2, 4.49f);
            }
    
            continue;
        }
    
        if(key == "windows_supported_versions")
        {
            if(line.Require(2))
            {
                windowsSupportedVersions[0] = line.GetFloat(1, 3.02f);
                windowsSupportedVersions[1] = line.GetFloat(2, 4.49f);
            }
    
            continue;
        }
    
        if(key == "ios_supported_versions")
        {
            if(line.Require(2))
            {
                iosSupportedVersions[0] = line.GetFloat(1, 3.02f);
                iosSupportedVersions[1] = line.GetFloat(2, 4.49f);
            }
    
            continue;
        }
    
        if(key == "macos_supported_versions")
        {
            if(line.Require(2))
            {
                macosSupportedVersions[0] = line.GetFloat(1, 3.02f);
                macosSupportedVersions[1] = line.GetFloat(2, 4.49f);
            }
    
            continue;
        }

        if(key == "enable_telnet_server")
        {
            if(!line.Require(1))
                return false;
            
            enableTelnetServer = line.GetUInt(1) == 1 ? true : false;
            continue;
        }

        /*if(key == "enable_world_balance")
        {
            if(!line.Require(1))
                return false;

            isWorldBalancerEnabled = line.GetInt(1) == 1 ? true : false;
        }

        if(key == "balance_world")
        {
            if(!line.Require(4))
                return false;

            WorldBalanceConfigSchema blnc;
            blnc.worldName = line.GetString(1);
            blnc.fileName = line.GetString(2);
            blnc.maxInstance = line.GetUInt(3);
            blnc.keepExactId = line.GetInt(4) == 1 ? true : false;

            balancedWorlds.push_back(std::move(blnc));
        }

        if(key == "balance_soft_cap_ratio")
        {
            balanceSoftCapRatio = line.GetFloat(1, 0.6f);
        }*/

        if(key == "max_npc_per_world")
        {
            maxNpcPerWorld = line.GetUInt(1, 20);
        }

        if(key == "net_panic_queue")
        {
            netThreshold.panicQueueSize = line.GetUInt(1);
        }

        if(key == "net_panic_permille")
        {
            netThreshold.panicCpuPermille = line.GetUInt(1);
        }

        if(key == "net_normal_burst")
        {
            netThreshold.normalBurst = line.GetUInt(1);
        }

        if(key == "net_heavy_burst")
        {
            netThreshold.heavyBurst = line.GetUInt(1);
        }

        if(key == "net_panic_burst")
        {
            netThreshold.panicBurst = line.GetUInt(1);
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
