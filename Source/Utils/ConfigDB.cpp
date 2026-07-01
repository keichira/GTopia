#include "ConfigDB.h"
#include "../IO/File.h"
#include "../IO/Log.h"
#include "StringUtils.h"

ConfigDB::ConfigDB() {}

ConfigDB::~ConfigDB() {}

bool ConfigLine::Has(int32 index) const
{
    return index < args.size();
}

const string& ConfigLine::GetString(int32 index, const string& def) const
{
    if (!Has(index))
        return def;

    return args[index];
}

int32 ConfigLine::GetInt(int32 index, int32 def) const
{
    if (!Has(index))
        return def;

    int32 val = def;
    if (ToInt(GetString(index), val) != TO_INT_SUCCESS)
    {
        dbParent->ReportError(*this, "ToInt failed (index " + ToString(index) + ")");
        return def;
    }

    return val;
}

uint32 ConfigLine::GetUInt(int32 index, uint32 def) const
{
    if (!Has(index))
        return def;

    uint32 val = def;
    if (ToUInt(GetString(index), val) != TO_INT_SUCCESS)
    {
        dbParent->ReportError(*this, "ToUInt failed (index " + ToString(index) + ")");
        return def;
    }

    return val;
}

float ConfigLine::GetFloat(int32 index, float def) const
{
    if (!Has(index))
        return def;

    return ToFloat(GetString(index));
}

bool ConfigLine::Require(int32 index) const
{
    if (!Has(index))
    {
        if (dbParent)
        {
            dbParent->ReportError(*this, "Expected " + ToString(index + 1) + " args but found " + ToString(args.size()) + " args");
        }
        return false;
    }

    return true;
}

bool ConfigLine::IsEmpty(int32 index) const
{
    if (!Has(index))
        return true;

    const string& str = GetString(index);
    return str.empty() || IsSpace(str[0]);
}

bool ConfigLine::IsLastSpace() const
{
    return IsEmpty(args.size() - 1);
}

bool ConfigDB::Load(const string& filePath)
{
    if (filePath.empty())
        return false;

    LOGGER_LOG_INFO_ASAP("ConfigDB: Loading file '%s'", filePath.c_str());

    File file;
    if (!file.Open(filePath))
    {
        LOGGER_LOG_ERROR_ASAP("ConfigDB: Failed to open file '%s'", filePath.c_str());
        return false;
    }

    string data;
    data.resize(file.GetSize());

    if (file.Read(data.data(), file.GetSize()) != file.GetSize())
    {
        file.Close();
        LOGGER_LOG_ERROR_ASAP("ConfigDB: Failed to read '%s'", filePath.c_str());
        return false;
    }
    file.Close();

    auto lines = Split(data, '\n');

    for (uint32 i = 0; i < lines.size(); ++i)
    {
        if (lines[i].empty() || IsSpace(lines[i][0]) || lines[i][0] == '#')
            continue;

        ConfigLine cl;
        cl.lineNumber = i + 1;
        cl.args = Split(lines[i], '|');
        cl.dbParent = this;

        for (auto& arg : cl.args)
        {
            StripWhiteSpace(arg);
        }

        m_lines.push_back(std::move(cl));
    }

    m_fileName = GetFileNameFromPath(filePath);
    LOGGER_LOG_INFO_ASAP("ConfigDB: Successfully loaded '%s' (%d lines)", filePath.c_str(), m_lines.size());
    return true;
}

void ConfigDB::ReportError(const ConfigLine& line, const string& msg)
{
    LOGGER_LOG_ERROR_ASAP("ConfigDB: Error in '%s' (line %d), %s", m_fileName.c_str(), line.lineNumber, msg.c_str());
}
