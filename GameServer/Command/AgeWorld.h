#pragma once

#include "CommandBase.h"

class AgeWorld : public CommandBase<AgeWorld> {
public:
    static const CommandInfo& GetInfo();

public:
    static void Execute(GamePlayer* pPlayer, std::vector<string>& args);
};