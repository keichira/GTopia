#include "ConfigDB.h"
#include "../IO/File.h"
#include "StringUtils.h"
#include "../IO/Log.h"

ConfigDB::ConfigDB()
{
}

ConfigDB::~ConfigDB()
{
}

bool ConfigLine::Has(int32 index) const
{
    return index < args.size();
}

const string& ConfigLine::GetString(int32 index, const string& def) const
{
    if(!Has(index))
        return def;

    return args[index];
}

int32 ConfigLine::GetInt(int32 index, int32 def) const
{
    if(!Has(index))
        return def;

    int32 val = def;
    if(ToInt(GetString(index), val) != TO_INT_SUCCESS)
    {
        LOGGER_LOG_ERROR("ToInt failed line: %d index: %d", lineNumber, index);
        return def;
    }

    return val;
}

uint32 ConfigLine::GetUInt(int32 index, uint32 def) const
{
    if(!Has(index))
        return def;

    uint32 val = def;
    if(ToUInt(GetString(index), val) != TO_INT_SUCCESS)
    {
        LOGGER_LOG_ERROR("ToUInt failed line: %d index: %d", lineNumber, index);
        return def;
    }

    return val;
}

float ConfigLine::GetFloat(int32 index, float def) const
{
    if(!Has(index))
        return def;

    return ToFloat(GetString(index));
}

bool ConfigLine::Require(int32 index) const
{
    if(!Has(index))
    {
        LOGGER_LOG_ERROR(
            "Line %d [%s]: expected %d got %d args", 
            lineNumber,
            args.empty() ? "UNKNOWN" : args[0].c_str(),
            index + 1,
            args.size()
        );
        return false;
    }

    return true;
}

bool ConfigDB::Load(const string &filePath)
{
    if(filePath.empty())
        return false;

    File file;
    if(!file.Open(filePath))
        return false;

    string data;
    data.resize(file.GetSize());

    if(file.Read(data.data(), file.GetSize()) != file.GetSize())
    {
        file.Close();
        return false;
    }
    file.Close();

    auto lines = Split(data, '\n');

    for(uint32 i = 0; i < lines.size(); ++i)
    {
        if(lines[i].empty() || lines[i][0] == '#')
            continue;

        ConfigLine cl;
        cl.lineNumber = i + 1;
        cl.args = Split(lines[i], '|');

        for(auto& arg : cl.args)
        {
            StripWhiteSpace(arg);
        }

        m_lines.push_back(std::move(cl));
    }

    return true;
}