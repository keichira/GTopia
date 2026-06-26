#pragma once

#include "CommandBase.h"

class FindItem : public CommandBase<FindItem> {
public:
    static const CommandInfo& GetInfo();

public:
    static void Execute(GamePlayer* pPlayer, std::vector<string>& args);
};