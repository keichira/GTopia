#pragma once

#include "CommandBase.h"

class Magic : public CommandBase<Magic>
{
public:
    static const CommandInfo& GetInfo();

public:
    static void Execute(GamePlayer* pPlayer, std::vector<string>& args);
};