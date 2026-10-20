#include "TCPEvent_Server.h"
#include "../../Context.h"
#include "../../Player/PlayerManager.h"
#include "../../Server/GameServer.h"
#include "../../Server/MasterBroadway.h"
#include "../../Store/StoreManager.h"
#include "Holiday/HolidayManager.h"
#include "IO/Log.h"
#include "Item/ItemInfoManager.h"
#include "Player/RoleManager.h"
#include "Server/StatsManager.h"
#include "TCPEvent_Server.h"
#include "Utils/StringUtils.h"

void TCPEvent_Hello(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    string authKey;
    if (!reader.ReadString(authKey))
        return;

    // XorCipher(authKey, SOCKET_AUTH_SECRET_KEY);

    LOGGER_LOG_INFO("Received hello packet from Master");
    GetMasterBroadway()->SendAuthPacket(authKey);
}

void TCPEvent_Auth(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    uint8 authed = 0;
    if (!reader.Read(authed))
        return;

    GetMasterBroadway()->SetAuthState(authed == 1 ? BROADWAY_AUTH_SUCCESS : BROADWAY_AUTH_FAILED);
}

void TCPEvent_HeartBeat(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    uint32 playerCount = 0;
    if (!reader.Read<uint32>(playerCount))
        return;

    GetPlayerManager()->SetTotalPlayerCount(playerCount);
    GetMasterBroadway()->SendHeartBeat();
}

void TCPEvent_KillServer(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    int32 timeSec = -1;
    reader.Read(timeSec);

    if (timeSec == 0)
        GetContext()->Shutdown();
    else
        GetGameServer()->ShutdownServersAfterSeconds(timeSec);
}

void TCPEvent_StatsUpdate(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    GetStatsManager()->OnTCPPacket(reader);
}

void TCPEvent_HolidaySync(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    GetHolidayManager()->OnTCPPacket(reader);
}

void TCPEvent_ReloadFile(NetClient* pClient, TCPPacketHeader& header, TCPPacketReader& reader)
{
    if (!pClient)
        return;

    string fileType;
    reader.ReadString(fileType);

    if (fileType == "config")
    {
        if (!GetContext()->GetGameConfig()->LoadConfig(GetProgramPath() + "/config.txt"))
        {
            LOGGER_LOG_ERROR("Received reload file packet but failed to reload config.txt");
            return;
        }

        LOGGER_LOG_INFO("Reloaded config.txt");
    }
    if (fileType == "news")
    {
        if (!GetContext()->GetGameConfig()->LoadNews(GetProgramPath() + "/news.txt"))
        {
            LOGGER_LOG_ERROR("Received reload file packet but failed to reload news.txt");
            return;
        }

        LOGGER_LOG_INFO("Reloaded news.txt");
    }
    else if (fileType == "playmods")
    {
        if (!GetPlayModManager()->Load(GetProgramPath() + "/playmods.txt"))
        {
            LOGGER_LOG_ERROR("Received reload file packet but failed to reload playmods.txt");
            return;
        }

        LOGGER_LOG_INFO("Reloaded playmods.txt");
    }
    else if (fileType == "roles")
    {
        if (!GetRoleManager()->Load(GetProgramPath() + "/roles.txt"))
        {
            LOGGER_LOG_ERROR("Received reload file packet but failed to reload roles.txt");
            return;
        }

        LOGGER_LOG_INFO("Reloaded roles.txt");
    }
    else if (fileType == "wiki")
    {
        if (!GetItemInfoManager()->LoadWikiData(GetProgramPath() + "/wiki_data.txt"))
        {
            LOGGER_LOG_ERROR("Received reload file packet but failed to reload wiki_data.txt");
            return;
        }

        LOGGER_LOG_INFO("Reloaded wiki_data.txt");
    }
    else if (fileType == "consumable")
    {
        if (!GetItemInfoManager()->LoadConsumableData(GetProgramPath() + "/consumable_data.txt"))
        {
            LOGGER_LOG_ERROR("Received reload file packet but failed to reload consumable_data.txt");
            return;
        }

        LOGGER_LOG_INFO("Reloaded consumable.txt");
    }
    else if (fileType == "store")
    {
        if (!GetStoreManager()->Load(GetProgramPath() + "/store.txt"))
        {
            LOGGER_LOG_ERROR("Received reload file packet but failed to reload store.txt");
            return;
        }

        LOGGER_LOG_INFO("Reloaded store.txt");
    }
}
