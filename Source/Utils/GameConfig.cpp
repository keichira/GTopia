#include "GameConfig.h"
#include "../IO/File.h"
#include "StringUtils.h"

#include "../IO/Log.h"

GameConfig::GameConfig()
{
}

bool GameConfig::LoadConfig(const string &filePath)
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

        if(args[0] == "cdn_info") {
            cdnServer = args[1];
            cdnPath = args[2];
        }
    }

    return true;
}

#ifdef SERVER_MASTER
uint16 GameConfig::LoadServers(const string& filePath)
#else
uint16 GameConfig::LoadServers(const string &filePath, uint16 serverID)
#endif
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

#ifndef SERVER_MASTER
    uint32 currServerIdEnd = 0;
#endif

    for(auto& line : lines) {
        if(line.empty() || line[0] == '#') {
            continue;
        }

        auto args = Split(line, '|');

        if(args[0] == "tcp_start") {
            tcpStart = (uint16)ToUInt(args[1]);
        }

        if(args[0] == "udp_start") {
            udpStart = (uint16)ToUInt(args[1]);
        }

        if(args[0] == "set_master") {
            uint16 masterServerID = 0;

            servers.emplace_back(
                ServerConfigSchema{
                    masterServerID, args[1], args[2],
                    tcpStart, udpStart
                }
            );
        }

        if(args[0] == "add_server") {
            uint16 serverCount = ToUInt(args[3]);
#ifdef SERVER_MASTER
            for(uint16 i = 0; i < serverCount; ++i) {
                uint16 serverID = servers.size();

                servers.emplace_back(
                    ServerConfigSchema{
                        serverID, args[1], args[2],
                        (uint16)(tcpStart + serverID), (uint16)(udpStart + serverID)
                    }
                );
            }
#else
            uint32 start = currServerIdEnd + 1;
            currServerIdEnd = start + serverCount + 1;

            if(serverID >= start && serverID <= currServerIdEnd) {
                servers.emplace_back(
                    ServerConfigSchema{
                        serverID, args[1], args[2],
                        (uint16)(tcpStart + serverID), (uint16)(udpStart + serverID)
                    }
                );

                break;
            }
#endif
        }
    }

    return servers.size();
}
