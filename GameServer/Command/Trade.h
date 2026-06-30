#pragma once

#include "CommandBase.h"

class Trade : public CommandBase<Trade> {
public:
    static const CommandInfo& GetInfo();

public:
    static void Execute(GamePlayer* pPlayer, std::vector<string>& args);
};