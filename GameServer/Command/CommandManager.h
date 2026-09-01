#pragma once

#include "../Player/GamePlayer.h"
#include "Event/EventDispatcher.h"
#include "Player/Role.h"
#include "Precompiled.h"

struct CommandInfo
{
    string usage = "";
    string desc = "";
    uint32 perm = 0;
    std::vector<uint32> aliases;
    bool enabled = true;
};

template <typename T> class CommandBase
{
public:
    static const CommandInfo& GetInfo() { return T::GetInfo(); }

    static void Execute(GamePlayer* pPlayer, std::vector<string>& args)
    {
        if (!CheckPerm(pPlayer))
            return;

        T::Execute(pPlayer, args);
    }

    static bool CheckPerm(GamePlayer* pPlayer)
    {
        if (!pPlayer)
            return false;

        Role* pRole = pPlayer->GetRole();
        if (!pRole || !pRole->HasPerm(GetInfo().perm) || !GetInfo().enabled)
        {
            pPlayer->SendOnConsoleMessage("`4Unknown command. ``Enter `$/help`` for a list of valid commands.");
            return false;
        }

        return true;
    }
};

class CommandManager
{
public:
    CommandManager() = default;
    ~CommandManager() = default;

public:
    static CommandManager* GetInstance()
    {
        static CommandManager instance;
        return &instance;
    }

public:
    void RegisterAllCommands();

    void ExecuteCommand(GamePlayer* pPlayer, std::vector<string>& args)
    {
        if (!pPlayer || args.empty())
            return;

        uint32 hashCmd = HashString(args[0].substr(1));
        if (!m_commands.HasHandler(hashCmd))
        {
            pPlayer->SendOnConsoleMessage("`4Unknown command. ``Enter `$/help`` for a list of valid commands.");
            return;
        }

        m_commands.Dispatch(hashCmd, pPlayer, args);
    }

private:
    template <class T> void Register()
    {
        for (auto& alias : T::GetInfo().aliases)
        {
            m_commands.Register(alias, Delegate<GamePlayer*, std::vector<string>&>::Create<&T::Execute>());
        }
    }

private:
    EventDispatcher<uint32, GamePlayer*, std::vector<string>&> m_commands;
};

CommandManager* GetCommandManager();

#define MAKE_COMMAND(Name, Usage, Desc, Perm, ...)                                                                     \
    class Command_##Name : public CommandBase<Command_##Name>                                                          \
    {                                                                                                                  \
    public:                                                                                                            \
        static const CommandInfo& GetInfo()                                                                            \
        {                                                                                                              \
            static CommandInfo info = {Usage, Desc, Perm, {__VA_ARGS__}, true};                                        \
            return info;                                                                                               \
        }                                                                                                              \
        static void Execute(GamePlayer* pPlayer, std::vector<string>& args);                                           \
    };                                                                                                                 \
    void Command_##Name::Execute(GamePlayer* pPlayer, std::vector<string>& args)