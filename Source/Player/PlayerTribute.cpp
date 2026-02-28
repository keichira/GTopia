#include "PlayerTribute.h"
#include "../IO/File.h"
#include "../Utils/StringUtils.h"
#include "../Memory/MemoryBuffer.h"
#include "../Proton/ProtonUtils.h"

PlayerTribute::PlayerTribute()
: m_dataVec(2)
{
}

PlayerTribute::~PlayerTribute()
{
}

bool PlayerTribute::Load(const string& filePath)
{
    File file;
    if(!file.Open(filePath)) {
        return false;
    }

    uint32 fileSize = file.GetSize();
    string fileData(fileSize, '\0');

    if(file.Read(fileData.data(), fileSize) != fileSize) {
        return false;
    }

    auto lines = Split(fileData, '\n');

    uint8 currHeader = 0;
    for(auto& line : lines) {
        if(line.empty() || line[0] == '#') {
            continue;
        }

        auto args = Split(line, '|');
        if(args[0] == "set_header") {
            if(args[1] == "epic_players") {
                currHeader = 0;
            }
            else if(args[1] == "exceptional_mentors") {
                currHeader = 1;
            }
            continue;
        }

        m_dataVec[currHeader] += line;
    }

    return true;
}

void PlayerTribute::SaveToClientData()
{
    uint32 memEstimate = 2 * 2;
    for(uint8 i = 0; i < m_dataVec.size(); ++i) {
        memEstimate += m_dataVec[i].size();
    }

    m_clientData.size = memEstimate;
    m_clientData.pData = new uint8[memEstimate];

    MemoryBuffer memBuffer(m_clientData.pData, memEstimate);
    
    for(uint8 i = 0; i < m_dataVec.size(); ++i) {
        memBuffer.WriteStringRaw(m_dataVec[i] + "\n");
    }

    m_clientData.hash = Proton::HashString((const char*)m_clientData.pData, m_clientData.size);
}

PlayerTribute* GetPlayerTributeManager() { return PlayerTribute::GetInstance(); }
