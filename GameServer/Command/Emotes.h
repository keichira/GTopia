#pragma once

#include "CommandBase.h"

class Emotes : public CommandBase<Emotes> {
public:
    static const CommandInfo& GetInfo();

public:
    static void Execute(GamePlayer* pPlayer, std::vector<string>& args);
};