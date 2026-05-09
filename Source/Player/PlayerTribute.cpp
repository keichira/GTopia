#include "PlayerTribute.h"
#include "../IO/File.h"
#include "../Utils/StringUtils.h"
#include "../Proton/ProtonUtils.h"

#include "../IO/Log.h"

PlayerTribute::PlayerTribute()
{
}

PlayerTribute::~PlayerTribute()
{
    for(uint8 i = 0; i < MAX_TRIBUTE_DATA_VERSION_COUNT; ++i)
    {
        SAFE_DELETE_ARRAY(m_clientData[i].pData);
    }
}

bool PlayerTribute::Load(const string& filePath)
{
    File file;
    if(!file.Open(filePath))
        return false;

    uint32 fileSize = file.GetSize();
    string fileData(fileSize, '\0');

    if(file.Read(fileData.data(), fileSize) != fileSize)
        return false;

    auto lines = Split(fileData, '\n');

    uint8 currHeader = 0;
    for(auto& line : lines) 
    {
        if(line.empty() || line[0] == '#')
            continue;

        auto args = Split(line, '|');

        if(args[0] == "set_header") 
        {
            if(args[1] == "epic_players") 
            {
                currHeader = 0;
            }
            else if(args[1] == "exceptional_mentors") 
            {
                currHeader = 1;
            }
            else if(args[1] == "charity_champions") 
            {
                currHeader = 2;
            }
            else if(args[1] == "grow_pass_leaders") 
            {
                currHeader = 3;
            }
            continue;
        }

        m_tributeData[currHeader] += line + "\n";
    }

    return true;
}

void PlayerTribute::SaveToClientData()
{
    for(uint8 i = 0; i < MAX_TRIBUTE_DATA_VERSION_COUNT; ++i)
    {
        MemoryBuffer memSize;
        Serialize(memSize, i);

        m_clientData[i].size = memSize.GetOffset();
        m_clientData[i].pData = new uint8[m_clientData[i].size];

        MemoryBuffer memBuffer(m_clientData[i].pData, m_clientData[i].size);
        Serialize(memBuffer, i);

        m_clientData[i].hash = Proton::HashString((const char*)m_clientData[i].pData, m_clientData[i].size);
    }
}

PlayerTributeClientData* PlayerTribute::GetClientData(uint32 protocol)
{
    if(protocol > 206)
        return &m_clientData[1];
    return &m_clientData[0];
}

void PlayerTribute::Serialize(MemoryBuffer& memBuffer, uint8 version)
{
    uint8 sizeToUse = 3;
    if(version == 1)
    {
        sizeToUse = 4;
    }

    for(uint8 i = 0; i < sizeToUse; ++i)
    {
        memBuffer.WriteRaw(&m_tributeData[i], m_tributeData[i].size());
    }
}

PlayerTribute* GetPlayerTributeManager() { return PlayerTribute::GetInstance(); }
