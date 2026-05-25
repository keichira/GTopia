#include "PlayModManager.h"
#include "../IO/File.h"
#include "../Utils/StringUtils.h"
#include "CharacterData.h"
#include "../Item/ItemInfoManager.h"
#include "../IO/Log.h"

uint32 StrToCharacterStateFlag(const string& flag)
{
    static const std::unordered_map<string, uint32> charFlagStrMap
    {
        {"CHAR_NO_CLIP", CHAR_STATE_NO_CLIP},
        {"CHAR_DOUBLE_JUMP", CHAR_STATE_DOUBLE_JUMP},
        {"CHAR_RENDER_EYES_ONLY", CHAR_STATE_RENDER_EYES_ONLY}
    };

    auto it = charFlagStrMap.find(flag);
    if(it != charFlagStrMap.end()) {
        return it->second;
    }

    return 0;
}

PlayModManager::PlayModManager()
{
}

PlayModManager::~PlayModManager()
{
}

bool PlayModManager::Load(const string &filePath)
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

    for(auto& line : lines) {
        if(line.empty() || line[0] == '#') {
            continue;
        }

        auto args = Split(line, '|');
        if(args[0] == "add_playmod") {
            PlayMod playMod;
            playMod.m_displayItem = (uint16)ToUInt(args[1]);
            playMod.m_name = args[2];
            playMod.m_addMessage = args[3];
            playMod.m_removeMessage = args[4];

            playMod.m_modType = ePlayModType(m_playMods.size() + 1);
            
            m_playMods.push_back(std::move(playMod));
        }

        if(args[0] == "set_char_flags") {
            for(uint8 i = 1; i < args.size(); ++i)
            {
                m_playMods.back().m_charState |= StrToCharacterStateFlag(args[i]);
            }
        }

        if(args[0] == "set_skin_color") {
            m_playMods.back().m_skinColor = ToColor(args[1], ',');
        }

        if(args[0] == "set_timer") {
            m_playMods.back().m_durationTime = ToUInt(args[1]);
        }

        if(args[0] == "set_punch_damage") {
            m_playMods.back().m_punchDamage = ToFloat(args[1]);
        }

        if(args[0] == "set_punch_power") {
            m_playMods.back().m_punchPower = ToFloat(args[1]);
        }

        if(args[0] == "set_build_range") {
            m_playMods.back().m_buildRange = ToUInt(args[1]);
        }

        if(args[0] == "set_speed") {
            m_playMods.back().m_speed = ToFloat(args[1]);
        }

        if(args[0] == "set_punch_type") {
            m_playMods.back().m_punchType = (uint8)ToUInt(args[1]);
        }

        if(args[0] == "set_items") {
            ItemInfoManager* pItemMgr = GetItemInfoManager();
            ePlayModType modType = m_playMods.back().m_modType;

            for(uint16 i = 1; i < args.size(); ++i) {
                uint32 itemID = 0;
                if(ToUInt(args[i], itemID) != TO_INT_SUCCESS) {
                    continue;
                }

                ItemInfo* pItem = pItemMgr->GetItemByID(itemID);
                if(!pItem) {
                    LOGGER_LOG_ERROR("Failed to setup playmod for item %d its not exists", itemID);
                    continue;
                }

                pItem->playModType = modType;
            }
        }
    }

    return true;
}

PlayMod* PlayModManager::GetPlayMod(ePlayModType type)
{
    if(type > m_playMods.size()) {
        return nullptr;
    }

    for(auto& playmod : m_playMods) {
        if(playmod.m_modType == type) {
            return &playmod;
        }
    }

    return nullptr;
}

PlayModManager* GetPlayModManager() { return PlayModManager::GetInstance(); }