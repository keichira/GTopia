#pragma once

#include "../Server/TelnetServer.h"
#include "Event/EventDispatcher.h"
#include "IO/Log.h"
#include "Precompiled.h"
#include "Utils/StringUtils.h"

struct TelnetCommandInfo
{
    string usage = "";
    string desc = "";
    int32 minAdminLevel = 999; // set it high
    std::vector<uint32> aliases;
};

template <typename T> class TelnetCommandBase
{
public:
    static const TelnetCommandInfo& GetInfo() { return T::GetInfo(); }

    static void Execute(TelnetClient* pNetClient, std::vector<string>& args)
    {
        if (!CheckPerm(pNetClient))
            return;

        T::Execute(pNetClient, args);
    }

    static bool CheckPerm(TelnetClient* pNetClient)
    {
        if (!pNetClient || pNetClient->GetAdminLevel() < GetInfo().minAdminLevel)
        {
            pNetClient->SendMessage("Unknown command.", true);
            return false;
        }

        return true;
    }

    static void SendUsage(TelnetClient* pNetClient)
    {
        if (!pNetClient)
            return;

        pNetClient->SendMessage("Command usage: " + GetInfo().usage, false);
    }
};

class TelnetCommandManager
{
public:
    TelnetCommandManager() = default;
    ~TelnetCommandManager() = default;

public:
    static TelnetCommandManager* GetInstance()
    {
        static TelnetCommandManager instance;
        return &instance;
    }

public:
    void RegisterAllCommands();

    void ExecuteCommand(TelnetClient* pNetClient, std::vector<string>& args)
    {
        if (!pNetClient)
            return;

        if (pNetClient->GetAdminLevel() == 0 || args.empty())
        {
            pNetClient->SendMessage("Unknown command.", true);
            return;
        }

        if (args[0].size() < 2 || args[0][0] != '/')
        {
            pNetClient->SendMessage("Unknown command. Commands must start with '/'", true);
            return;
        }

        uint32 hashCmd = HashString(args[0].substr(1));
        if (!m_commands.HasHandler(hashCmd))
        {
            pNetClient->SendMessage("Unknown command.", true);
            return;
        }

        LOGGER_LOG_INFO("[Telnet] Client IP: %s Name: %s executed: %s", pNetClient->GetIP().c_str(),
                        pNetClient->GetDisplayName().c_str(), JoinString(args, " ").c_str());

        m_commands.Dispatch(hashCmd, pNetClient, args);
    }

private:
    template <class T> void Register()
    {
        for (auto& alias : T::GetInfo().aliases)
        {
            m_commands.Register(alias, Delegate<TelnetClient*, std::vector<string>&>::Create<&T::Execute>());
        }
    }

private:
    EventDispatcher<uint32, TelnetClient*, std::vector<string>&> m_commands;
};

TelnetCommandManager* GetTelnetCommandManager();

#define MAKE_COMMAND(Name, Usage, Desc, Perm, ...)                                                                     \
    class Command_##Name : public TelnetCommandBase<Command_##Name>                                                    \
    {                                                                                                                  \
    public:                                                                                                            \
        static const TelnetCommandInfo& GetInfo()                                                                      \
        {                                                                                                              \
            static TelnetCommandInfo info = {Usage, Desc, Perm, {__VA_ARGS__}};                                        \
            return info;                                                                                               \
        }                                                                                                              \
        static void Execute(TelnetClient* pNetClient, std::vector<string>& args);                                      \
    };                                                                                                                 \
    void Command_##Name::Execute(TelnetClient* pNetClient, std::vector<string>& args)