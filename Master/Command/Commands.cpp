#include "../Context.h"
#include "../Player/PlayerManager.h"
#include "../Server/ServerManager.h"
#include "CommandManager.h"
#include "Database/Table/PlayerDBTable.h"
#include "Holiday/HolidayManager.h"
#include "Player/PlayerPlayMod.h"
#include "Player/RoleManager.h"
#include "Utils/GrowUtils.h"
#include <unordered_set>

static void SetRoleCB(QueryTaskResult&& result)
{
    CommandContext* pCtx = GetAdminCommandManager()->GetPendingContext(result.ownerID);
    if (!pCtx)
        return;

    if (result.status != QUERY_STATUS_OK)
    {
        pCtx->SendReply("Failed to execute database query.");
    }
    else if (result.affectedRowCount == 0)
    {
        pCtx->SendReply("User not found or role was already set to this ID.");
    }
    else
    {
        pCtx->SendReply("Successfully updated player's role.");

        Variant* pRoleID = result.GetExtraData(0);
        Variant* pUserID = result.GetExtraData(1);

        if (pRoleID && pUserID)
        {
            PlayerSession* pPlayerSession = GetPlayerManager()->GetSessionByID(pUserID->GetUINT());
            if (pPlayerSession)
            {
                ServerInfo* pServer = GetServerManager()->GetServerByID(pPlayerSession->serverID);
                if (pServer)
                {
                    GetServerManager()->SendCommandSetRole(pServer, pPlayerSession->userID, pRoleID->GetUINT());
                }
            }
        }
    }

    GetAdminCommandManager()->RemovePendingContext(result.ownerID);
}

MAKE_COMMAND(SetRole, "/setrole <userID> <roleID>", "Update player's role", 4, CMD_SOURCE_TELNET | CMD_SOURCE_DISCORD,
             "setrole"_hash)
{
    if (ctx.args.size() < 3)
    {
        SendUsage(ctx);
        return;
    }

    uint32 userID = 0;
    if (ToUInt(ctx.args[1], userID) != TO_INT_SUCCESS || userID == 0)
    {
        ctx.SendReply("userID must be valid number.");
        return;
    }

    uint32 roleID = 0;
    if (ToUInt(ctx.args[2], roleID) != TO_INT_SUCCESS)
    {
        ctx.SendReply("roleID must be valid number.");
        return;
    }

    if (!GetRoleManager()->GetRole(roleID))
    {
        ctx.SendReply("roleID is not valid role.");
        return;
    }

    uint64 reqID = GetAdminCommandManager()->RegisterPendingContext(ctx);

    QueryRequest req = PlayerDB::SetRoleByID(roleID, userID, reqID);
    req.AddExtraData(roleID, userID);
    req.callback = &SetRoleCB;

    DatabasePlayerExec(GetContext()->GetDatabasePool(), req);
}

MAKE_COMMAND(Shutdown, "/shutdown [timeSec (-1 to stop shutdown)]", "Shutdown all game servers and master server", 4,
             CMD_SOURCE_TELNET, "shutdown"_hash)
{
    int32 timeSec = 0;
    if (ctx.args.size() > 1)
    {
        if (ToInt(ctx.args[1], timeSec) != TO_INT_SUCCESS)
            ctx.SendReply("timeSec must be number.");
    }

    ctx.SendReply("Initiating server shutdown sequence, time: " + ToString(timeSec) + " seconds.");
    GetServerManager()->ShutdownAllServers(timeSec);
}

static void GivePlayModOfflineCB(QueryTaskResult&& result)
{
    CommandContext* pCtx = GetAdminCommandManager()->GetPendingContext(result.ownerID);
    if (!pCtx)
        return;

    if (result.status != QUERY_STATUS_OK || !result.result || result.result->GetRowCount() == 0)
    {
        pCtx->SendReply("Player not found in database.");
        GetAdminCommandManager()->RemovePendingContext(result.ownerID);
        return;
    }

    DatabaseResult* dbRes = result.result;

    uint32 userID = dbRes->GetField("ID", 0).GetUINT();
    if (userID == 0)
    {
        pCtx->SendReply("Fetched userID is invalid.");
        GetAdminCommandManager()->RemovePendingContext(result.ownerID);
        return;
    }

    const string& name = dbRes->GetField("Name", 0).GetString();
    const string& guestName = dbRes->GetField("GuestName", 0).GetString();
    int32 guestID = dbRes->GetField("GuestID", 0).GetINT();
    int32 roleID = dbRes->GetField("RoleID", 0).GetINT();

    const string& dbPlayMods = dbRes->GetField("PlayMods", 0).GetString();
    const string& ip = dbRes->GetField("IP", 0).GetString();
    const string& mac = dbRes->GetField("Mac", 0).GetString();
    const string& gid = dbRes->GetField("GID", 0).GetString();
    const string& sid = dbRes->GetField("SID", 0).GetString();
    const string& vid = dbRes->GetField("VID", 0).GetString();
    int32 hash = dbRes->GetField("Hash", 0).GetINT();
    int32 platformType = dbRes->GetField("PlatformType", 0).GetINT();

    int32 playModType = result.GetExtraData(0)->GetINT();
    int32 durationMin = result.GetExtraData(1)->GetUINT();
    int32 punishType = result.GetExtraData(2)->GetUINT();
    uint32 adminUserID = result.GetExtraData(3)->GetUINT();
    const string& adminName = result.GetExtraData(4)->GetString();
    bool isSilent = result.GetExtraData(5)->GetINT() == 1;
    const string& reason = result.GetExtraData(6)->GetString();

    PlayerPlayMod playMod;
    if (!dbPlayMods.empty())
    {
        MemoryBuffer memBuffer(dbPlayMods.data(), dbPlayMods.size());
        playMod.Serialize(memBuffer, false);
    }

    string targetName = GetPlayerRawName(name, guestName, guestID);
    string formattedName = GetPlayerDisplayName(targetName, roleID, false, false, false);

    string statusMsg, broadcastMsg;
    GivePlayerPlayModAndOther(GetContext()->GetDatabasePool(), playMod, playModType, durationMin, punishType, true,
                              userID, targetName, formattedName, adminUserID, adminName, ip, mac, sid, vid, gid, hash,
                              platformType, reason, isSilent, statusMsg, broadcastMsg);

    MemoryBuffer playModBuffer(playMod.GetMemEstimate());
    playMod.Serialize(playModBuffer, true);
    string updatedPlayMods((char*)playModBuffer.GetData(), playModBuffer.GetOffset());

    QueryRequest req = PlayerDB::UpdatePlaymods(updatedPlayMods, userID);
    DatabasePlayerExec(GetContext()->GetDatabasePool(), req);

    pCtx->SendReply(statusMsg);
    GetAdminCommandManager()->RemovePendingContext(result.ownerID);
}

static void HandlePlayModCommand(CommandContext& ctx, int32 playModType, const string& usageStr)
{
    if (ctx.args.size() < 3)
    {
        ctx.SendReply("Usage: " + usageStr);
        return;
    }

    uint32 targetUserID = 0;
    if (ToUInt(ctx.args[1], targetUserID) != TO_INT_SUCCESS || targetUserID == 0)
    {
        ctx.SendReply("userID must be a valid number.");
        return;
    }

    uint32 durationMin = 0;
    if (ToUInt(ctx.args[2], durationMin) != TO_INT_SUCCESS)
    {
        ctx.SendReply("durationMin must be a valid number.");
        return;
    }

    uint32 punishType = PLAYER_PUNISH_SOFT;
    if (ctx.args.size() > 3)
    {
        if (ctx.args[3] == "ip")
            punishType = PLAYER_PUNISH_IP;
        else if (ctx.args[3] == "hard")
            punishType = PLAYER_PUNISH_HARD;
        else if (ctx.args[3] == "soft")
            punishType = PLAYER_PUNISH_SOFT;
        else if (ctx.args[3] == "device")
            punishType = PLAYER_PUNISH_DEVICE;
    }

    bool isSilent = false;
    if (ctx.args.size() > 4)
    {
        isSilent = (ctx.args[4] == "true" ? true : false);
    }

    string reason = "";
    if (ctx.args.size() > 5)
    {
        for (int32 i = 5; i < ctx.args.size(); ++i)
        {
            reason += (i == 5 ? "" : " ") + ctx.args[i];
        }
    }

    uint32 adminUserID = 0;
    string adminName = "UNKNOWN";

    if (ctx.source == CMD_SOURCE_GAMESERVER)
    {
        adminUserID = ctx.adminNetID;
        adminName = "#" + ToString(adminUserID);
    }
    else if (ctx.source == CMD_SOURCE_DISCORD)
    {
        adminUserID = 0;
        adminName = "DISCORD";
    }
    else if (ctx.source == CMD_SOURCE_TELNET)
    {
        adminUserID = 0;
        adminName = "TELNET";
    }

    uint64 reqID = GetAdminCommandManager()->RegisterPendingContext(ctx);

    if (PlayerSession* pSession = GetPlayerManager()->GetSessionByID(targetUserID))
    {
        ServerInfo* pServer = GetServerManager()->GetServerByID(pSession->serverID);
        if (pServer && pServer->pClient)
        {
            GetServerManager()->SendCommandGivePlayMod(pServer, reqID, targetUserID, playModType, durationMin,
                                                       punishType, adminUserID, isSilent, reason);
        }
        else
        {
            ctx.SendReply("GameServer connection lost.");
            GetAdminCommandManager()->RemovePendingContext(reqID);
        }
    }
    else
    {
        QueryRequest req = PlayerDB::GetPlaymodPunishData(targetUserID, reqID);
        req.AddExtraData(playModType, durationMin, punishType, adminUserID, adminName, (int32)(isSilent ? 1 : 0),
                         reason);
        req.callback = &GivePlayModOfflineCB;

        DatabasePlayerExec(GetContext()->GetDatabasePool(), req);
    }
}

MAKE_COMMAND(PBan, "/pban <userID> <durationMin> [ip/hard/soft/device] [isSilent (true/false)] [reason]",
             "Ban a player", 3, CMD_SOURCE_ALL, "pban"_hash)
{
    HandlePlayModCommand(ctx, PLAYMOD_TYPE_BAN, GetInfo().usage);
}

MAKE_COMMAND(Mute, "/mute <userID> <durationMin> [ip/hard/soft/device] [isSilent (true/false)] [reason]",
             "Duct-tape a player's mouth", 2, CMD_SOURCE_ALL, "mute"_hash)
{
    HandlePlayModCommand(ctx, PLAYMOD_TYPE_DUCT_TAPE, GetInfo().usage);
}

MAKE_COMMAND(Curse, "/curse <userID> <durationMin> [ip/hard/soft/device] [isSilent (true/false)] [reason]",
             "Curse a player to netherworld", 2, CMD_SOURCE_ALL, "curse"_hash)
{
    HandlePlayModCommand(ctx, PLAYMOD_TYPE_CURSE, GetInfo().usage);
}

MAKE_COMMAND(SetPlayMod, "/setplaymod <userID> <durationMin> <playmodID>", "Add playmod to player", 3, CMD_SOURCE_ALL,
             "setplaymod"_hash)
{
    if (ctx.args.size() != 4)
    {
        SendUsage(ctx);
        return;
    }

    uint32 playModId = 0;
    if (ToUInt(ctx.args[3], playModId) != TO_INT_SUCCESS)
    {
        ctx.SendReply("playmodID must be number.");
        return;
    }

    if (playModId == 0)
    {
        ctx.SendReply("PlayModID 0 is not really a playmod...");
        return;
    }

    if (playModId > GetPlayModManager()->GetPlayModSize())
    {
        ctx.SendReply("There arent that much playmods.");
        return;
    }

    if (playModId == PLAYMOD_TYPE_BAN || playModId == PLAYMOD_TYPE_CURSE || playModId == PLAYMOD_TYPE_DUCT_TAPE)
    {
        ctx.SendReply("These type playmods are not allowed from this command.");
        return;
    }

    ctx.args.pop_back();
    HandlePlayModCommand(ctx, playModId, GetInfo().usage);
}

MAKE_COMMAND(Help, "/help", "List available commands for your admin level", 1, CMD_SOURCE_TELNET | CMD_SOURCE_DISCORD,
             "help"_hash)
{
    int32 adminLevel = 0;
    AdminClient* pAdmin = GetAdminServer()->GetClientByNetID(ctx.adminNetID);
    if (pAdmin)
    {
        adminLevel = pAdmin->GetAdminLevel();
    }

    // maybe preinitialize commands to list on help, but for now this

    const auto& cmdInfos = GetAdminCommandManager()->GetCommandInfos();
    std::unordered_set<const AdminCommandInfo*> processedInfos;

    string reply = "=== Available Commands ===\n";
    bool foundAny = false;

    for (auto& [_, pInfo] : cmdInfos)
    {
        if (!pInfo)
            continue;

        if (processedInfos.find(pInfo) != processedInfos.end())
            continue;

        processedInfos.insert(pInfo);

        if ((pInfo->sourceFlags & ctx.source) == 0)
            continue;

        if (adminLevel >= pInfo->minAdminLevel)
        {
            reply += pInfo->usage + " - " + pInfo->desc + "\n";
            foundAny = true;
        }
    }

    if (!foundAny)
    {
        ctx.SendReply("No commands available for your admin level.");
        return;
    }

    ctx.SendReply(reply);
}

MAKE_COMMAND(Lua, "/lua <subcommand> [args...]", "Manage lua feature", 4, CMD_SOURCE_DISCORD | CMD_SOURCE_TELNET,
             "lua"_hash)
{
    if (ctx.args.size() < 2)
    {
        ctx.SendReply("=== Lua Management Help ===\n"
                      "Usage: /lua <subcommand> [args...]\n\n"
                      "  /lua reload <script_name|*> [server_id]\n"
                      "  /lua load <script_name> [server_id]\n"
                      "  /lua unload <script_name> [server_id]\n"
                      "  /lua profile <on|off|reset> [server_id]\n"
                      "  /lua profile dump <server_id> [time|avg|max|calls|mem] [top_n]\n"
                      "No server_id = all game servers.");
        return;
    }

    string sub = ToLower(ctx.args[1]);
    ServerManager* pServerManager = GetServerManager();
    AdminCommandManager* pCommandManager = GetAdminCommandManager();

    uint8 action = LUA_ACTION_PROFILE_OFF;
    string arg = "";
    uint32 number = 0;
    usize serverArgIndex = 0;

    if (sub == "reload" || sub == "load" || sub == "unload")
    {
        if (ctx.args.size() < 3)
        {
            string target = sub == "reload" ? "<script_name|*>" : "<script_name>";
            ctx.SendReply("Usage: /lua " + sub + " " + target + " [server_id]");
            return;
        }

        if (sub == "reload")
        {
            action = LUA_ACTION_RELOAD;
        }
        else if (sub == "load")
        {
            action = LUA_ACTION_LOAD;
        }
        else
        {
            action = LUA_ACTION_UNLOAD;
        }

        arg = ctx.args[2];
        serverArgIndex = 3;
    }
    else if (sub == "profile")
    {
        if (ctx.args.size() < 3)
        {
            ctx.SendReply("Usage: /lua profile <on|off|reset|dump> [args...]");
            return;
        }

        string mode = ToLower(ctx.args[2]);

        if (mode == "on")
        {
            action = LUA_ACTION_PROFILE_ON;
            serverArgIndex = 3;
        }
        else if (mode == "off")
        {
            action = LUA_ACTION_PROFILE_OFF;
            serverArgIndex = 3;
        }
        else if (mode == "reset")
        {
            action = LUA_ACTION_PROFILE_RESET;
            serverArgIndex = 3;
        }
        else if (mode == "dump")
        {
            if (ctx.args.size() < 4)
            {
                ctx.SendReply("Usage: /lua profile dump <server_id> [time|avg|max|calls|mem] [top_n]");
                return;
            }

            number = 15;
            if (ctx.args.size() > 5 && (ToUInt(ctx.args[5], number) != TO_INT_SUCCESS || number == 0 || number > 50))
            {
                ctx.SendReply("top_n must be a number between 1 and 50.");
                return;
            }

            action = LUA_ACTION_PROFILE_DUMP;
            arg = ctx.args.size() > 4 ? ToLower(ctx.args[4]) : "time";
            serverArgIndex = 3;
        }
        else
        {
            ctx.SendReply("Unknown profile mode. Use on, off, reset or dump.");
            return;
        }
    }
    else
    {
        ctx.SendReply("Unknown subcommand. Type '/lua' without arguments to see the help menu.");
        return;
    }

    ServerInfo* pServer = nullptr;
    if (ctx.args.size() > serverArgIndex)
    {
        uint32 serverID = 0;
        if (ToUInt(ctx.args[serverArgIndex], serverID) != TO_INT_SUCCESS || serverID == 0 || serverID > 65535)
        {
            ctx.SendReply("serverID must be a positive number.");
            return;
        }

        pServer = pServerManager->GetServerByID(serverID);
        if (!pServer || !pServer->pClient || pServer->serverType != CONFIG_SERVER_GAME)
        {
            ctx.SendReply("Game server " + ToString(serverID) + " is not connected.");
            return;
        }
    }
    else if (!pServerManager->HasAnyGameServer())
    {
        ctx.SendReply("No game servers are connected.");
        return;
    }

    uint64 reqID = pCommandManager->RegisterPendingContext(ctx);
    pServerManager->SendCommandLua(pServer, reqID, action, arg, number);
}

MAKE_COMMAND(Holiday, "/holiday <subcommand> [args...]", "Manage holidays and holiday rules", 3, CMD_SOURCE_ALL,
             "holiday"_hash)
{
    if (ctx.args.size() < 2)
    {
        ctx.SendReply("=== Holiday Management Help ===\n"
                      "Usage: /holiday <subcommand> [args...]\n\n"
                      "Subcommands:\n"
                      "  list\n"
                      "  info <key>\n"
                      "  start <key>\n"
                      "  end <key>\n"
                      "  create <key> <main|auto> <durationDays> [time] [sched]\n"
                      "  delete <key>\n"
                      "  setduration <key> <days>\n"
                      "  settime <key> <time>\n"
                      "  settype <key> <main|auto>\n"
                      "  setsched <key> <none|unix|monthly|yearly|random> [param]\n"
                      "  addrule <key> <start|end> <statKey> <op> <threshold>\n"
                      "  delrule <key> <statKey> <start|end>\n"
                      "  clearrules <key>\n\n"
                      "Parameter Hints:\n"
                      "  - Types: main, auto\n"
                      "  - Schedules: none, unix, monthly, yearly, random\n"
                      "  - Operators: >= (ge), > (gt), <= (le), < (lt), == (eq)");
        return;
    }

    string sub = ToLower(ctx.args[1]);

    if (sub == "list")
    {
        auto& holidays = GetHolidayManager()->GetHolidays();
        if (holidays.empty())
        {
            ctx.SendReply("No holidays found in the system.");
            return;
        }

        string reply = "=== Registered Holidays (" + ToString(holidays.size()) + ") ===\n";
        for (auto& [key, pHoliday] : holidays)
        {
            if (!pHoliday)
                continue;
            const HolidayData& data = pHoliday->GetData();
            reply += " - " + key + " | Status: " + (data.isActive ? "ACTIVE" : "INACTIVE") +
                     " | Type: " + (data.type == HOLIDAY_TYPE_MAIN ? "MAIN" : "AUTO") +
                     " | Duration: " + ToString(data.durationDays) + "d" + " | Rules: " + ToString(data.rules.size()) +
                     "\n";
        }
        ctx.SendReply(reply);
    }
    else if (sub == "info")
    {
        if (ctx.args.size() < 3)
        {
            ctx.SendReply("Usage: /holiday info <holidayKey>");
            return;
        }

        string key = ToLower(ctx.args[2]);
        BaseHoliday* pHoliday = GetHolidayManager()->GetHoliday(key);
        if (!pHoliday)
        {
            ctx.SendReply("Error: Holiday '" + key + "' not found.");
            return;
        }

        HolidayData& data = pHoliday->GetData();
        uint32 effectiveStart = data.startTimeUnix;

        time_t now = time(nullptr);
        int32 mon = 0, d = 0, y = 0, h = 0, min = 0, sec = 0;
        GetTimeLocal(&now, &mon, &d, &y, &h, &min, &sec);

        if (data.scheduleType == HOLIDAY_SCHED_MONTHLY && data.schedDay > 0)
        {
            tm timeinfo;
            timeinfo.tm_year = y - 1900;
            timeinfo.tm_mon = mon - 1;
            timeinfo.tm_mday = data.schedDay;
            effectiveStart = (uint32)mktime(&timeinfo);
        }
        else if (data.scheduleType == HOLIDAY_SCHED_YEARLY && data.schedMonth > 0 && data.schedDay > 0)
        {
            tm timeinfo;
            timeinfo.tm_year = y - 1900;
            timeinfo.tm_mon = data.schedMonth - 1;
            timeinfo.tm_mday = data.schedDay;
            effectiveStart = (uint32)mktime(&timeinfo);
        }

        uint32 endTimeUnix = effectiveStart + (data.durationDays * 86400);

        DiscordEmbed embed;
        embed.title = "Holiday Info: " + key;
        embed.color = data.isActive ? 0x57F287 : 0xED4245;
        embed.AddField({"ID", ToString(data.id), true});
        embed.AddField({"Status", data.isActive ? "ACTIVE" : "INACTIVE", true});
        embed.AddField({"Type", data.type == HOLIDAY_TYPE_MAIN ? "MAIN" : "AUTO", true});
        embed.AddField({"Duration", ToString(data.durationDays) + " days", true});

        if (effectiveStart == 0)
        {
            embed.AddField({"Start Time", "Not set", true});
            embed.AddField({"End Time", "Not set", true});
        }
        else
        {
            embed.AddField({"Start Time",
                            Time::FormatTimeToStr((time_t)effectiveStart) + " (" + ToString(effectiveStart) + ")",
                            true});
            embed.AddField(
                {"End Time", Time::FormatTimeToStr((time_t)endTimeUnix) + " (" + ToString(endTimeUnix) + ")", true});
        }

        static const char* schedNames[] = {"NONE", "UNIX", "MONTHLY", "YEARLY", "RANDOM"};
        embed.AddField({"Schedule Type", data.scheduleType <= 4 ? schedNames[data.scheduleType] : "UNKNOWN", true});
        embed.AddField({"Schedule Param", data.scheduleParam.empty() ? "None" : data.scheduleParam, true});
        embed.AddField({"Rules Count", ToString(data.rules.size()), true});

        ctx.SendEmbed(embed);
    }
    else if (sub == "start")
    {
        if (ctx.args.size() < 3)
        {
            ctx.SendReply("Usage: /holiday start <holidayKey>");
            return;
        }

        string key = ToLower(ctx.args[2]);
        BaseHoliday* pHoliday = GetHolidayManager()->GetHoliday(key);
        if (!pHoliday)
        {
            ctx.SendReply("Error: Holiday '" + key + "' not found.");
            return;
        }

        if (pHoliday->IsActive())
        {
            ctx.SendReply("Holiday '" + key + "' is already active.");
            return;
        }

        GetHolidayManager()->StartHoliday(key);
        ctx.SendReply("Holiday '" + key + "' started successfully.");
    }
    else if (sub == "end")
    {
        if (ctx.args.size() < 3)
        {
            ctx.SendReply("Usage: /holiday end <holidayKey>");
            return;
        }

        string key = ToLower(ctx.args[2]);
        BaseHoliday* pHoliday = GetHolidayManager()->GetHoliday(key);
        if (!pHoliday)
        {
            ctx.SendReply("Error: Holiday '" + key + "' not found.");
            return;
        }

        if (!pHoliday->IsActive())
        {
            ctx.SendReply("Holiday '" + key + "' is already inactive.");
            return;
        }

        GetHolidayManager()->EndHoliday(key);
        ctx.SendReply("Holiday '" + key + "' ended successfully.");
    }
    else if (sub == "create")
    {
        if (ctx.args.size() < 5)
        {
            ctx.SendReply(
                "Usage: /holiday create <key> <main|auto> <durationDays> [startTime|now|+1d] [schedType] [schedParam]");
            return;
        }
        string key = ToLower(ctx.args[2]);

        eHolidayType holidayType = HOLIDAY_TYPE_MAIN;
        string typeStr = ToLower(ctx.args[3]);
        if (typeStr == "main" || typeStr == "0")
            holidayType = HOLIDAY_TYPE_MAIN;
        else if (typeStr == "auto" || typeStr == "1")
            holidayType = HOLIDAY_TYPE_AUTO;
        else
        {
            ctx.SendReply("Error: Invalid holiday type. Please use 'main' or 'auto'.");
            return;
        }

        uint32 durationDays = 1;
        if (ToUInt(ctx.args[4], durationDays) != TO_INT_SUCCESS || durationDays == 0)
        {
            ctx.SendReply("Error: Duration days must be a positive integer.");
            return;
        }

        uint32 startTimeUnix = 0;
        if (ctx.args.size() > 5)
        {
            string timeArg = ToLower(ctx.args[5]);
            if (timeArg != "0")
            {
                uint32 now = (uint32)Time::GetTimeSinceEpoch();
                if (timeArg == "now")
                {
                    startTimeUnix = now;
                }
                else
                {
                    uint32 directUnix = 0;
                    if (ToUInt(timeArg, directUnix) == TO_INT_SUCCESS && directUnix > 100000000)
                    {
                        startTimeUnix = directUnix;
                    }
                    else
                    {
                        int32 startIdx = (timeArg[0] == '+') ? 1 : 0;
                        char unit = timeArg.back();
                        string valStr = timeArg.substr(startIdx, timeArg.size() - startIdx - (IsAlpha(unit) ? 1 : 0));

                        uint32 offsetValue = 0;
                        if (ToUInt(valStr, offsetValue) != TO_INT_SUCCESS)
                        {
                            ctx.SendReply("Error: Invalid start time format. Use Unix timestamp, 'now', or relative "
                                          "format like '+1d', '+12h', '30m'.");
                            return;
                        }

                        uint32 secondsMultiplier = 1;
                        if (unit == 'm')
                            secondsMultiplier = 60;
                        else if (unit == 'h')
                            secondsMultiplier = 3600;
                        else if (unit == 'd')
                            secondsMultiplier = 86400;
                        else if (unit == 'w')
                            secondsMultiplier = 604800;

                        startTimeUnix = now + (offsetValue * secondsMultiplier);
                    }
                }
            }
        }

        eHolidayScheduleType schedType = HOLIDAY_SCHED_NONE;
        if (ctx.args.size() > 6)
        {
            string s = ToLower(ctx.args[6]);
            if (s == "none" || s == "0")
                schedType = HOLIDAY_SCHED_NONE;
            else if (s == "unix" || s == "unixtime" || s == "1")
                schedType = HOLIDAY_SCHED_UNIX_TIME;
            else if (s == "monthly" || s == "2")
                schedType = HOLIDAY_SCHED_MONTHLY;
            else if (s == "yearly" || s == "3")
                schedType = HOLIDAY_SCHED_YEARLY;
            else if (s == "random" || s == "4")
                schedType = HOLIDAY_SCHED_RANDOM;
            else
            {
                ctx.SendReply("Error: Invalid schedule type. Choose from: [none | unix | monthly | yearly | random].");
                return;
            }
        }

        string schedParam = (ctx.args.size() > 7 ? ctx.args[7] : "");

        HolidayData data;
        data.holidayKey = key;
        data.type = holidayType;
        data.durationDays = durationDays;
        data.startTimeUnix = startTimeUnix;
        data.scheduleType = schedType;
        data.scheduleParam = schedParam;
        data.isActive = false;

        if (GetHolidayManager()->AdminCreateHoliday(data))
        {
            ctx.SendReply("Holiday '" + key + "' created successfully.");
        }
        else
        {
            ctx.SendReply("Error: Holiday '" + key + "' already exists.");
        }
    }
    else if (sub == "delete")
    {
        if (ctx.args.size() < 3)
        {
            ctx.SendReply("Usage: /holiday delete <holidayKey>");
            return;
        }
        string key = ToLower(ctx.args[2]);
        if (GetHolidayManager()->AdminDeleteHoliday(key))
        {
            ctx.SendReply("Holiday '" + key + "' and its associated rules have been deleted.");
        }
        else
        {
            ctx.SendReply("Error: Holiday not found.");
        }
    }
    else if (sub == "setduration")
    {
        if (ctx.args.size() < 4)
        {
            ctx.SendReply("Usage: /holiday setduration <holidayKey> <durationDays>");
            return;
        }

        string key = ToLower(ctx.args[2]);
        uint32 duration = 0;
        if (ToUInt(ctx.args[3], duration) != TO_INT_SUCCESS || duration == 0)
        {
            ctx.SendReply("Error: Duration days must be a valid positive integer.");
            return;
        }

        if (GetHolidayManager()->SetHolidayDuration(key, duration))
        {
            ctx.SendReply("Holiday '" + key + "' duration updated to " + ToString(duration) + " days.");
        }
        else
        {
            ctx.SendReply("Error: Holiday not found.");
        }
    }
    else if (sub == "settime")
    {
        if (ctx.args.size() < 4)
        {
            ctx.SendReply("Usage: /holiday settime <holidayKey> <startTimeUnix | now | +1d | +12h | 30m>");
            return;
        }
        string key = ToLower(ctx.args[2]);
        string timeArg = ToLower(ctx.args[3]);
        uint32 unixTime = 0;

        if (timeArg == "0")
        {
            unixTime = 0;
        }
        else
        {
            uint32 now = (uint32)Time::GetTimeSinceEpoch();
            if (timeArg == "now")
            {
                unixTime = now;
            }
            else
            {
                uint32 directUnix = 0;
                if (ToUInt(timeArg, directUnix) == TO_INT_SUCCESS && directUnix > 100000000)
                {
                    unixTime = directUnix;
                }
                else
                {
                    int32 startIdx = (timeArg[0] == '+') ? 1 : 0;
                    char unit = timeArg.back();
                    string valStr = timeArg.substr(startIdx, timeArg.size() - startIdx - (IsAlpha(unit) ? 1 : 0));

                    uint32 offsetValue = 0;
                    if (ToUInt(valStr, offsetValue) != TO_INT_SUCCESS)
                    {
                        ctx.SendReply("Error: Invalid time format. Use Unix timestamp, 'now', or relative format like "
                                      "'+1d', '+12h', '30m'.");
                        return;
                    }

                    uint32 secondsMultiplier = 1;
                    if (unit == 'm')
                        secondsMultiplier = 60;
                    else if (unit == 'h')
                        secondsMultiplier = 3600;
                    else if (unit == 'd')
                        secondsMultiplier = 86400;
                    else if (unit == 'w')
                        secondsMultiplier = 604800;

                    unixTime = now + (offsetValue * secondsMultiplier);
                }
            }
        }

        if (GetHolidayManager()->SetHolidayUnixTime(key, unixTime))
        {
            ctx.SendReply("Holiday '" + key + "' start time updated to: " + ToString(unixTime));
        }
        else
        {
            ctx.SendReply("Error: Holiday not found.");
        }
    }
    else if (sub == "settype")
    {
        if (ctx.args.size() < 4)
        {
            ctx.SendReply("Usage: /holiday settype <holidayKey> <main|auto>");
            return;
        }
        string key = ToLower(ctx.args[2]);
        string typeStr = ToLower(ctx.args[3]);
        eHolidayType holidayType = HOLIDAY_TYPE_MAIN;

        if (typeStr == "main" || typeStr == "0")
            holidayType = HOLIDAY_TYPE_MAIN;
        else if (typeStr == "auto" || typeStr == "1")
            holidayType = HOLIDAY_TYPE_AUTO;
        else
        {
            ctx.SendReply("Error: Invalid type. Please use 'main' or 'auto'.");
            return;
        }

        if (GetHolidayManager()->SetHolidayType(key, holidayType))
        {
            ctx.SendReply("Holiday '" + key +
                          "' type updated to: " + (holidayType == HOLIDAY_TYPE_AUTO ? "AUTO" : "MAIN"));
        }
        else
        {
            ctx.SendReply("Error: Holiday not found.");
        }
    }
    else if (sub == "setsched")
    {
        if (ctx.args.size() < 4)
        {
            ctx.SendReply("Usage: /holiday setsched <holidayKey> <none|unix|monthly|yearly|random> [param]");
            return;
        }
        string key = ToLower(ctx.args[2]);
        string s = ToLower(ctx.args[3]);
        eHolidayScheduleType schedType = HOLIDAY_SCHED_NONE;

        if (s == "none" || s == "0")
            schedType = HOLIDAY_SCHED_NONE;
        else if (s == "unix" || s == "unixtime" || s == "1")
            schedType = HOLIDAY_SCHED_UNIX_TIME;
        else if (s == "monthly" || s == "2")
            schedType = HOLIDAY_SCHED_MONTHLY;
        else if (s == "yearly" || s == "3")
            schedType = HOLIDAY_SCHED_YEARLY;
        else if (s == "random" || s == "4")
            schedType = HOLIDAY_SCHED_RANDOM;
        else
        {
            ctx.SendReply("Error: Invalid schedule type. Choose from: [none | unix | monthly | yearly | random].");
            return;
        }

        string param = (ctx.args.size() > 4 ? ctx.args[4] : "");

        if (GetHolidayManager()->SetHolidaySchedule(key, schedType, param))
        {
            ctx.SendReply("Holiday '" + key + "' schedule updated successfully.");
        }
        else
        {
            ctx.SendReply("Error: Holiday not found.");
        }
    }
    else if (sub == "addrule")
    {
        if (ctx.args.size() < 7)
        {
            ctx.SendReply("Usage: /holiday addrule <holidayKey> <start|end> <statKey> <>=|>=|<=|<|==> <threshold>\n"
                          "Operators supported: >= (ge), > (gt), <= (le), < (lt), == (eq)");
            return;
        }
        string key = ToLower(ctx.args[2]);

        string ruleTypeStr = ToLower(ctx.args[3]);
        uint8 ruleType = 0;
        if (ruleTypeStr == "start" || ruleTypeStr == "0")
            ruleType = RULE_TYPE_START;
        else if (ruleTypeStr == "end" || ruleTypeStr == "1")
            ruleType = RULE_TYPE_END;
        else
        {
            ctx.SendReply("Error: Invalid rule type. Use 'start' or 'end'.");
            return;
        }

        string statKey = ToLower(ctx.args[4]);

        string opStr = ToLower(ctx.args[5]);
        eRuleOperator op = RULE_OP_GREATER_EQUAL;
        if (opStr == ">=" || opStr == "ge" || opStr == "0")
            op = RULE_OP_GREATER_EQUAL;
        else if (opStr == ">" || opStr == "gt" || opStr == "1")
            op = RULE_OP_GREATER;
        else if (opStr == "<=" || opStr == "le" || opStr == "2")
            op = RULE_OP_LESS_EQUAL;
        else if (opStr == "<" || opStr == "lt" || opStr == "3")
            op = RULE_OP_LESS;
        else if (opStr == "==" || opStr == "=" || opStr == "eq" || opStr == "4")
            op = RULE_OP_EQUAL;
        else
        {
            ctx.SendReply("Error: Invalid operator. Supported operators: >=, >, <=, <, ==");
            return;
        }

        int32 threshold = 0;
        if (ToInt(ctx.args[6], threshold) != TO_INT_SUCCESS)
        {
            ctx.SendReply("Error: Threshold must be a valid integer.");
            return;
        }

        HolidayRule rule;
        rule.holidayKey = key;
        rule.ruleType = ruleType;
        rule.statKey = statKey;
        rule.operatorType = op;
        rule.threshold = threshold;

        if (GetHolidayManager()->AddHolidayRule(key, rule))
        {
            ctx.SendReply("Rule successfully added to holiday '" + key + "'.");
        }
        else
        {
            ctx.SendReply("Error: Holiday not found.");
        }
    }
    else if (sub == "delrule")
    {
        if (ctx.args.size() < 5)
        {
            ctx.SendReply("Usage: /holiday delrule <holidayKey> <statKey> <start|end>");
            return;
        }
        string key = ToLower(ctx.args[2]);
        string statKey = ToLower(ctx.args[3]);

        string ruleTypeStr = ToLower(ctx.args[4]);
        uint8 ruleType = 0;
        if (ruleTypeStr == "start" || ruleTypeStr == "0")
            ruleType = RULE_TYPE_START;
        else if (ruleTypeStr == "end" || ruleTypeStr == "1")
            ruleType = RULE_TYPE_END;
        else
        {
            ctx.SendReply("Error: Invalid rule type. Use 'start' or 'end'.");
            return;
        }

        if (GetHolidayManager()->DeleteHolidayRule(key, statKey, ruleType))
        {
            ctx.SendReply("Rule '" + statKey + "' removed from holiday '" + key + "'.");
        }
        else
        {
            ctx.SendReply("Error: Rule or Holiday not found.");
        }
    }
    else if (sub == "clearrules")
    {
        if (ctx.args.size() < 3)
        {
            ctx.SendReply("Usage: /holiday clearrules <holidayKey>");
            return;
        }
        string key = ToLower(ctx.args[2]);
        if (GetHolidayManager()->ClearHolidayRules(key))
        {
            ctx.SendReply("All rules cleared for holiday '" + key + "'.");
        }
        else
        {
            ctx.SendReply("Error: Holiday not found.");
        }
    }
    else
    {
        ctx.SendReply("Unknown subcommand. Type '/holiday' without arguments to see the help menu.");
    }
}

MAKE_COMMAND(CReload, "/creload <config|news|playmods|roles|wiki|consumable|store>",
             "Reload configuration files for all servers", 4, CMD_SOURCE_TELNET, "creload"_hash)
{
    if (ctx.args.size() < 2)
    {
        SendUsage(ctx);
        return;
    }

    if (ctx.args[1] != "config" && ctx.args[1] != "news" && ctx.args[1] != "playmods" && ctx.args[1] != "wiki" &&
        ctx.args[1] != "consumable" && ctx.args[1] != "store" && ctx.args[1] != "roles")
    {
        ctx.SendReply("Unknown config" + ctx.args[1]);
        return;
    }

    if (ctx.args[1] == "config")
    {
        if (!GetContext()->GetGameConfig()->LoadConfig(GetProgramPath() + "/config.txt"))
            LOGGER_LOG_ERROR("Received reload file packet but failed to reload config.txt")
        else
            LOGGER_LOG_INFO("Reloaded config.txt");
    }
    else if (ctx.args[1] == "playmods")
    {
        if (!GetPlayModManager()->Load(GetProgramPath() + "/playmods.txt"))
            LOGGER_LOG_ERROR("Received reload file packet but failed to reload playmods.txt")
        else
            LOGGER_LOG_INFO("Reloaded playmods.txt")
    }
    else if (ctx.args[1] == "roles")
    {
        if (!GetRoleManager()->Load(GetProgramPath() + "/roles.txt"))
            LOGGER_LOG_ERROR("Received reload file packet but failed to reload roles.txt")
        else
            LOGGER_LOG_INFO("Reloaded roles.txt")
    }

    GetServerManager()->SendReloadFilePacketToAll(ctx.args[1]);
    ctx.SendReply("Sent all servers to reload config '" + ctx.args[1] + "'");
}

// for now only send to discord bot
MAKE_COMMAND(OnlineStats, "/onlinestats", "Send online stats such as player and world count", 4, CMD_SOURCE_DISCORD,
             "onlinestats"_hash)
{
    ctx.SendReply(ToString(GetPlayerManager()->GetTotalPlayerCount()));
}

void AdminCommandManager::RegisterAllCommands()
{
    Register<Command_SetRole>();
    Register<Command_Shutdown>();
    Register<Command_PBan>();
    Register<Command_Mute>();
    Register<Command_Curse>();
    Register<Command_SetPlayMod>();
    Register<Command_Help>();
    Register<Command_Holiday>();
    Register<Command_CReload>();
    Register<Command_Lua>();

    Register<Command_OnlineStats>(); // todo
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
///////////////// ADD COMMANDS ON TOP ONLY ///////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

AdminCommandManager::AdminCommandManager() : m_requestCounter(0) {}
AdminCommandManager::~AdminCommandManager() {}

AdminCommandManager* GetAdminCommandManager()
{
    return AdminCommandManager::GetInstance();
}

void AdminCommandManager::ExecuteCommand(CommandContext& ctx)
{
    if (ctx.args.empty() || ctx.args[0].empty())
        return;

    string cmdName = ctx.args[0];
    if (cmdName[0] == '/' || cmdName[0] == '!')
        cmdName = cmdName.substr(1);

    uint32 cmdHash = HashString(cmdName);

    if (!m_dispatcher.HasHandler(cmdHash))
    {
        ctx.SendReply("Error: Unknown command '" + cmdName + "'.");
        return;
    }

    auto it = m_commandInfos.find(cmdHash);
    if (it != m_commandInfos.end() && it->second)
    {
        if ((it->second->sourceFlags & ctx.source) == 0)
        {
            ctx.SendReply("Error: Command not found.");
            return;
        }
    }

    if (ctx.source == CMD_SOURCE_TELNET)
    {
        if (AdminClient* pAdminClient = GetAdminServer()->GetClientByNetID(ctx.adminNetID))
        {
            LOGGER_LOG_INFO("[Admin] %s executed %s", pAdminClient->GetDisplayName().c_str(),
                            JoinString(ctx.args, " ").c_str());
        }
    }

    m_dispatcher.Dispatch(cmdHash, ctx);
}

uint64 AdminCommandManager::RegisterPendingContext(CommandContext& ctx)
{
    if (m_pendingCommands.empty())
        m_requestCounter = 0;

    m_requestCounter++;
    ctx.requestID = m_requestCounter;
    ctx.creationTime.Reset();

    if (ctx.source == CMD_SOURCE_TELNET)
    {
        AdminClient* pAdmin = GetAdminServer()->GetClientByNetID(ctx.adminNetID);
        if (pAdmin)
            pAdmin->SetBusy(true);
    }

    m_pendingCommands[ctx.requestID] = ctx;
    return ctx.requestID;
}

CommandContext* AdminCommandManager::GetPendingContext(uint64 requestID)
{
    auto it = m_pendingCommands.find(requestID);
    if (it != m_pendingCommands.end())
        return &it->second;
    return nullptr;
}

void AdminCommandManager::RemovePendingContext(uint64 requestID)
{
    auto it = m_pendingCommands.find(requestID);
    if (it == m_pendingCommands.end())
        return;

    CommandContext& ctx = it->second;
    if (ctx.source == CMD_SOURCE_TELNET)
    {
        AdminClient* pAdmin = GetAdminServer()->GetClientByNetID(ctx.adminNetID);
        if (pAdmin)
            pAdmin->SetBusy(false);
    }

    m_pendingCommands.erase(it);
}

void AdminCommandManager::ProcessTimeouts()
{
    if (m_pendingCommands.empty() || m_lastTimeoutCheckTimer.GetElapsedTime() < 2000)
        return;

    std::vector<uint64> timedOutIDs;

    for (auto& [reqID, ctx] : m_pendingCommands)
    {
        if (ctx.creationTime.GetElapsedTime() >= 5000)
        {
            timedOutIDs.push_back(reqID);
        }
    }

    for (uint64 reqID : timedOutIDs)
    {
        auto it = m_pendingCommands.find(reqID);
        if (it != m_pendingCommands.end())
        {
            CommandContext& ctx = it->second;
            ctx.SendReply("Request timed out (No response from DB or GameServer).");
            RemovePendingContext(reqID);
        }
    }

    m_lastTimeoutCheckTimer.Reset();
}