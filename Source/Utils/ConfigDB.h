#pragma once

#include "../Precompiled.h"

class ConfigLine {
public:
    int32 lineNumber;
    std::vector<string> args;

    bool Has(int32 index) const;
    const string& GetString(int32 index, const string& def = "") const;
    int32 GetInt(int32 index, int32 def = 0) const;
    uint32 GetUInt(int32 index, uint32 def = 0) const;
    float GetFloat(int32 index, float def = 0.0f) const;
    bool Require(int32 index) const;

    uint32 GetArgSize() const { return args.size(); }
};

class ConfigDB {
public:
    ConfigDB();
    ~ConfigDB();

public:
    bool Load(const string& filePath);
    const std::vector<ConfigLine>& Lines() const { return m_lines; }

private:
    std::vector<ConfigLine> m_lines;
};