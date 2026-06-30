#pragma once

#include "TelnetCommandBase.h"

class SetRole : public TelnetCommandBase<SetRole>
{
public:
    static const TelnetCommandInfo& GetInfo();

public:
    static void Execute(TelnetClient* pNetClient, std::vector<string>& args);
};